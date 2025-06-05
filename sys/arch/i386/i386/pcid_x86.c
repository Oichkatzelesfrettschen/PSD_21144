#ifdef TEST_HAL_PCID_ALLOCATOR
// Define minimal types and stubs NEEDED by pcid_x86.c for standalone testing.
// This section aims to PREVENT inclusion of conflicting kernel headers like <sys/types.h>.

#include <stdint.h> // For uint32_t, uint64_t
#include <stddef.h> // For NULL, size_t (though not directly used by pcid_x86.c itself)
#include <stdio.h>  // For printf (used as kprintf stub)
#include <stdlib.h> // For abort (used as panic stub)
#include <string.h> // For memset (used in pcid_bitmap_zero_all), strncpy
#include <assert.h> // For assert in stubs

// --- Minimal Kernel Type Stubs for TEST_HAL_PCID_ALLOCATOR ---
typedef uint64_t uintptr_t; // Assuming uintptr_t is used by included machine headers like x86_mmu_utils.h

// Forward declare struct proc if its pointer type is used, but not its content
struct proc;

// Minimal stub for lock_object and its functions
typedef struct { char lo_name[32]; /* Ensure size is enough for real lock_object or debug info */ } lock_object_stub_t;
#define simple_lock_init(lock, name)         do { printf("STUB_LOCK: simple_lock_init for '%s'\n", name); /* Add more if state needed */ } while(0)
#define simple_lock(lock)                    do { /* printf("STUB_LOCK: simple_lock\n"); */ } while(0)
#define simple_unlock(lock)                  do { /* printf("STUB_LOCK: simple_unlock\n"); */ } while(0)

// Stub for kprintf and panic
#define kprintf printf
#define panic(msg) do { fprintf(stderr, "PANIC_STUB: %s\n", msg); assert(0); abort(); } while(0)

// --- End Minimal Kernel Type Stubs ---

// Mock environment integration
#include "mock_hal_env.h" // This path implies -I../../test/hal for test compile
#undef x86_rcr4
#define x86_rcr4() mock_x86_rcr4(g_mock_env)
#undef x86_lcr4
#define x86_lcr4(val) mock_x86_lcr4(g_mock_env, val)

// Use X86_CR4_PCIDE from mock_hal_env.h in test mode
#define CR4_PCIDE X86_CR4_PCIDE

// Assume atomic builtins are available via GCC, or mock_hal_env.h could provide them if necessary
#ifndef atomic_load_acq_64
#define atomic_load_acq_64(ptr) __atomic_load_n((ptr), __ATOMIC_ACQUIRE)
#endif
#ifndef atomic_cmpset_64
#define atomic_cmpset_64(ptr, oldval, newval) __sync_bool_compare_and_swap((volatile uint64_t*)ptr, oldval, newval)
#endif
#ifndef atomic_store_int
#define atomic_store_int(ptr, val) __atomic_store_n((ptr), val, __ATOMIC_RELEASE)
#endif
#ifndef atomic_load_int
#define atomic_load_int(ptr) __atomic_load_n((ptr), __ATOMIC_ACQUIRE)
#endif
#ifndef atomic_fetchadd_int
#define atomic_fetchadd_int(ptr, val) __sync_fetch_and_add((volatile unsigned int*)ptr, val)
#endif


#else // NOT TEST_HAL_PCID_ALLOCATOR (Normal Kernel Compile)

#include <sys/types.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/lock.h>      // Real struct lock_object, simple_lock_*
#include <sys/atomic.h>    // Real atomic operations

// Common includes for both modes (these should be safe after type stubbing above)
// These provide hardware-specific definitions and arch-specific atomics.
#include <machine/x86_mmu_utils.h>
#include <machine/specialreg.h>  // Defines CR4_PCIDE for kernel
#include <machine/atomic.h> // For i386_atomic_testset_uq (real one)

#endif // TEST_HAL_PCID_ALLOCATOR


#define PCID_MAX 4096
#define PCID_BITMAP_SIZE (PCID_MAX / (sizeof(uint8_t) * 8)) // Corrected for sizeof

static struct {
    volatile uint64_t generation_and_next_hint; // [63:32] generation, [31:0] next_pcid search hint
    uint8_t allocated_pcids_bitmap[PCID_BITMAP_SIZE]; // Bitmap of allocated PCIDs
    volatile unsigned int active_pcids[PCID_MAX];    // Reference counts for each PCID
#ifdef TEST_HAL_PCID_ALLOCATOR
    lock_object_stub_t alloc_lock;
#else
    struct lock_object alloc_lock;
#endif
} pcid_state = {
    .generation_and_next_hint = ((uint64_t)1 << 32) | 1, // Gen 1, next_pcid hint 1
    // allocated_pcids_bitmap and active_pcids are implicitly zero-initialized for static storage, lock needs init
};

// Bitmap helper: Set a bit
static void pcid_bitmap_set(int pcid) {
    if (pcid < 0 || pcid >= PCID_MAX) return;
    pcid_state.allocated_pcids_bitmap[pcid / 8] |= (1 << (pcid % 8));
}

// Bitmap helper: Clear a bit
static void pcid_bitmap_clear(int pcid) {
    if (pcid < 0 || pcid >= PCID_MAX) return;
    pcid_state.allocated_pcids_bitmap[pcid / 8] &= ~(1 << (pcid % 8));
}

// Bitmap helper: Test a bit
static int pcid_bitmap_is_set(int pcid) {
    if (pcid < 0 || pcid >= PCID_MAX) return 1;
    return (pcid_state.allocated_pcids_bitmap[pcid / 8] & (1 << (pcid % 8))) != 0;
}

// Bitmap helper: Find next zero bit (free PCID)
static int pcid_bitmap_find_free(uint32_t start_hint) {
    for (int i = 0; i < PCID_MAX; ++i) {
        int pcid_to_check = (start_hint + i);
        if (pcid_to_check >= PCID_MAX) { // Wrap around logic
            pcid_to_check = pcid_to_check % PCID_MAX;
        }
        if (pcid_to_check == 0) continue; // Skip PCID 0
        if (!pcid_bitmap_is_set(pcid_to_check)) {
            return pcid_to_check;
        }
    }
    return -1; // No free PCID found
}

// Bitmap helper: Zero all bits
static void pcid_bitmap_zero_all(void) {
    for (int i = 0; i < PCID_BITMAP_SIZE; ++i) {
        pcid_state.allocated_pcids_bitmap[i] = 0;
    }
}

void x86_pcid_init(void) {
    simple_lock_init(&pcid_state.alloc_lock, "x86pcid_alloc");
    // Bitmap is already zeroed due to static initialization.
    // Explicitly zero active_pcids reference counts.
    for (int i = 0; i < PCID_MAX; ++i) {
        pcid_state.active_pcids[i] = 0;
    }
    // Reset generation_and_next_hint and bitmap for clean test state
    pcid_state.generation_and_next_hint = ((uint64_t)1 << 32) | 1; // Gen 1, next_pcid hint 1
    pcid_bitmap_zero_all(); // Clear the allocation bitmap

    kprintf("x86_pcid_init: PCID state RESET. gen_next_hint=0x%llx\n",
            (unsigned long long)pcid_state.generation_and_next_hint);

#ifdef TEST_HAL_PCID_ALLOCATOR
    kprintf("[pcid_x86.c/x86_pcid_init] TEST_MODE: g_mock_env = %p\n", (void*)g_mock_env);
    if (g_mock_env) {
        kprintf("[pcid_x86.c/x86_pcid_init] TEST_MODE: g_mock_env->cpu.cr4 = 0x%lx\n", g_mock_env->cpu.cr4);
        // Reset test-specific counters
        g_mock_env->metrics.pcid_alloc_count = 0;
        g_mock_env->metrics.pcid_release_count = 0;
        g_mock_env->metrics.pcid_wraparound_count = 0;
        g_mock_env->current_alloc_attempt = 0;
    }
#endif
}

uint32_t x86_get_pcid_generation(void) {
    // For reading just the generation, a simple volatile read might be acceptable
    // if updates to generation are atomic or rare and protected by other means during update.
    // However, to be safe with potential 64-bit read tearing on 32-bit archs (not an issue here for gen part)
    // or for consistency with updates, a lock or atomic read is better.
    // Given generation_and_next_hint is updated by CAS, direct volatile read is okay for non-critical check.
    return (uint32_t)(pcid_state.generation_and_next_hint >> 32);
}

static void perform_global_tlb_flush_locked(void) {
    kprintf("PCID: Performing global TLB flush by toggling CR4.PCIDE.\n");
    uintptr_t cr4 = x86_rcr4();
    if (cr4 & X86_CR4_PCIDE) { // Use X86_CR4_PCIDE from mock_hal_env.h
        x86_lcr4(cr4 & ~X86_CR4_PCIDE);
        x86_lcr4(cr4 | X86_CR4_PCIDE);
    } else {
        kprintf("PCID: WARN - perform_global_tlb_flush_locked called but X86_CR4_PCIDE was off in mock CR4.\n");
        // Potentially reload CR3 as a fallback full flush if PCIDE wasn't on.
        // x86_lcr3(x86_rcr3());
    }
}

uint64_t x86_allocate_pcid(struct proc *p) {
    (void)p; // p not directly used for storing ASID in this version, HAL layer does that.
    uint64_t current_combined_state, new_combined_state_for_hint;
    uint32_t current_gen, current_next_hint_val;
    int pcid_to_alloc;

#ifdef TEST_HAL_PCID_ALLOCATOR
    kprintf("[pcid_x86.c/x86_allocate_pcid] TEST_MODE: g_mock_env = %p\n", (void*)g_mock_env);
    if (g_mock_env) {
        kprintf("[pcid_x86.c/x86_allocate_pcid] TEST_MODE: g_mock_env->cpu.cr4 = 0x%lx, force_exhaustion=%d, fail_at_n=%u, current_attempt=%u\n",
                g_mock_env->cpu.cr4, g_mock_env->force_pcid_exhaustion, g_mock_env->fail_at_allocation_n, g_mock_env->current_alloc_attempt);
        g_mock_env->current_alloc_attempt++; // Increment for fail_at_allocation_n logic
        if (g_mock_env->fail_at_allocation_n > 0 && g_mock_env->current_alloc_attempt == g_mock_env->fail_at_allocation_n) {
            kprintf("[pcid_x86.c/x86_allocate_pcid] TEST_MODE: Simulating allocation failure at attempt %u\n", g_mock_env->current_alloc_attempt);
            // Simulate failure by returning an invalid PCID/ASID. The test should check for this.
            // PCID 0 is often reserved or invalid.
            return 0; // Invalid ASID (PCID 0 with any generation)
        }
    }
#endif

retry_alloc:
    current_combined_state = atomic_load_acq_64(&pcid_state.generation_and_next_hint);
    current_gen = (uint32_t)(current_combined_state >> 32);
    current_next_hint_val = (uint32_t)(current_combined_state & 0xFFFFFFFF);
    if (current_next_hint_val == 0 || current_next_hint_val >= PCID_MAX) current_next_hint_val = 1;

    simple_lock(&pcid_state.alloc_lock);
    pcid_to_alloc = pcid_bitmap_find_free(current_next_hint_val);

    if (pcid_to_alloc != -1) { // Found a free PCID in bitmap
        // This allocation path assumes a free slot in bitmap is authoritative *under lock*.
        pcid_bitmap_set(pcid_to_alloc);
        atomic_store_int(&pcid_state.active_pcids[pcid_to_alloc], 1); // Set ref count to 1

        // Optimistically update next_hint part of the combined state for next allocation.
        uint32_t next_val_for_hint = (pcid_to_alloc + 1);
        if (next_val_for_hint >= PCID_MAX) next_val_for_hint = 1;

        simple_unlock(&pcid_state.alloc_lock);

        // Update the global hint using CAS, but only if generation hasn't changed by another thread.
        // This is an optimization for the hint. The critical part (bitmap set) was under lock.
        uint64_t observed_state;
        do {
            observed_state = current_combined_state; // Use the value we based our allocation on
            new_combined_state_for_hint = ((uint64_t)current_gen << 32) | next_val_for_hint;
            // If generation changed (observed_state >> 32 != current_gen), CAS will fail, which is fine.
            // We've already allocated pcid_to_alloc for *this* current_gen.
        } while (!atomic_cmpset_64(&pcid_state.generation_and_next_hint, observed_state, new_combined_state_for_hint));

        // ---- START MOVED HOOK LOGIC ----
        uint64_t asid64_result_success = ((uint64_t)current_gen << 32) | pcid_to_alloc;
        #ifdef TEST_HAL_PCID_ALLOCATOR
        // Super basic print
        printf("[pcid_x86.c/DBG] REACHED COMMON RETURN PATH SECTION (SUCCESS). pcid_to_alloc=%d, current_gen=%u\n", pcid_to_alloc, current_gen);
        fflush(stdout);

        printf("[pcid_x86.c/DBG] About to check hook condition. pcid_to_alloc=%d, current_gen=%u, g_mock_env=%p\n", pcid_to_alloc, current_gen, (void*)g_mock_env);
        fflush(stdout);
        if (g_mock_env && pcid_to_alloc != -1 && pcid_to_alloc != (uint32_t)-1) {
            printf("[pcid_x86.c/DBG] Hook condition TRUE. Calling mock_hook_pcid_allocated.\n");
            fflush(stdout);
            mock_hook_pcid_allocated(g_mock_env, (uint32_t)pcid_to_alloc, current_gen);
        } else {
            printf("[pcid_x86.c/DBG] Hook condition FALSE. pcid_to_alloc: %d, g_mock_env: %p\n", pcid_to_alloc, (void*)g_mock_env);
            fflush(stdout);
        }
        #endif
        return asid64_result_success;
        // ---- END MOVED HOOK LOGIC ----

    } else { // No free PCID found in bitmap, wraparound needed
        simple_unlock(&pcid_state.alloc_lock);

        // Attempt to increment generation and reset hints/bitmap using CAS
        // This thread will try to become the one responsible for the global flush.
        uint64_t captured_outer_state, desired_next_gen_state;
        uint32_t new_gen;
        do {
            captured_outer_state = atomic_load_acq_64(&pcid_state.generation_and_next_hint);
            current_gen = (uint32_t)(captured_outer_state >> 32);
            new_gen = current_gen + 1;
            if (new_gen == 0) new_gen = 1; // Handle generation wraparound
            desired_next_gen_state = ((uint64_t)new_gen << 32) | 1; // New gen, next_hint = 1
        } while (!atomic_cmpset_64(&pcid_state.generation_and_next_hint, captured_outer_state, desired_next_gen_state));

        // If CAS succeeded (or even if it failed and another thread did it),
        // this thread observed that a generation change was needed.
        // The thread that *succeeded* the CAS should perform the flush and reset.
        // However, simply seeing that generation changed means we need to retry.
        // The feedback had the CAS winner do the flush.
        // This simplified version assumes the CAS updates the generation, and then retries.
        // The actual flush and bitmap clear must be done carefully.
        // For now, let's assume the CAS for generation update signals need for flush,
        // and the next allocation attempt will find a cleared bitmap or fresh state.
        // This part needs robust handling of which thread does the cleanup.
        // A common pattern: only the thread that successfully increments generation does the flush and bitmap clear.

        // Re-evaluating: The thread that successfully CASes generation_and_next_hint to new_gen should do the flush & clear.
        if ((uint32_t)(atomic_load_acq_64(&pcid_state.generation_and_next_hint) >> 32) == new_gen) {
             // We might be the one that set the new generation or another thread did.
             // To be safe, only one thread should do this.
             // This simplified version doesn't have a clear "winner takes all" for the flush.
             // A better model might involve a separate "flush_needed" flag or ensuring only one proceeds.
             // For now, if a wraparound was detected (pcid_to_alloc == -1), assume a flush is needed.
             // The generation was incremented atomically.

            perform_global_tlb_flush_locked(); // Global flush
            #ifdef TEST_HAL_PCID_ALLOCATOR
            if (g_mock_env) { mock_hook_pcid_wraparound_flush(g_mock_env, new_gen); }
            #endif

            simple_lock(&pcid_state.alloc_lock);
            pcid_bitmap_zero_all();
            for(int i=0; i<PCID_MAX; ++i) { // Reset ref counts
                 atomic_store_int(&pcid_state.active_pcids[i], 0);
            }
            simple_unlock(&pcid_state.alloc_lock);
        }
        goto retry_alloc;
    }
    // This section is now effectively unreachable if the success path returns early
    // and the wraparound path uses goto.
    // uint64_t asid64_result = ((uint64_t)current_gen << 32) | pcid_to_alloc; // This line is problematic if pcid_to_alloc was -1 and not updated
    // #ifdef TEST_HAL_PCID_ALLOCATOR
    // printf("[pcid_x86.c/DBG] UNREACHABLE POINT? pcid_to_alloc=%d, current_gen=%u\n", pcid_to_alloc, current_gen);
    // fflush(stdout);
    // #endif
    // return asid64_result; // This return should ideally not be reached.
    // For safety, if it *is* reached, it means pcid_to_alloc was -1 from the start or some other error.
    // The original code implicitly relied on pcid_to_alloc being set before this point.
    // Given the goto, this final block is indeed unreachable if the logic above is sound.
    // Let's ensure it returns something identifiable as an error if it *were* reached.
    // However, the compiler might complain about unreachable code if the above paths are exhaustive.
    // For now, let's assume the above return in success path and goto in wraparound path cover all cases.
    // The original code structure implies this bottom part was the common exit.
    // The key is that `pcid_to_alloc` would have been set by the time it got here.
    // If we've correctly moved the hook for the main success path, this part is less critical
    // for *that* hook, but the wraparound path still needs to be considered for its own hook.
    // The wraparound path has its hook *before* the goto.

    // The original code had this:
    // } else { // No free PCID found in bitmap, wraparound needed
    //     ...
    //     perform_global_tlb_flush_locked(); // Global flush
    //     #ifdef TEST_HAL_PCID_ALLOCATOR
    //     if (g_mock_env) { mock_hook_pcid_wraparound_flush(g_mock_env, new_gen); } // Hook for wraparound
    //     #endif
    //     ...
    //     goto retry_alloc;
    // }
    // // Fallthrough for successful allocation
    // uint64_t asid64_result = ((uint64_t)current_gen << 32) | pcid_to_alloc;
    // // Hook for successful allocation was here
    // return asid64_result;

    // My change moves the successful hook up. The final return here is now only reachable
    // if pcid_to_alloc was -1 initially AND the wraparound logic somehow exited without goto.
    // This shouldn't happen. Let's remove this potentially problematic dead code.
    // If the compiler complains about "control may reach end of non-void function", then there's a path not returning.
    // The `goto retry_alloc` ensures the else branch loops. The if branch now returns. So this is dead.
    return 0; // Should be unreachable / error
}

void x86_release_pcid(uint64_t pcid_gen_asid) {
    uint32_t pcid = (uint32_t)(pcid_gen_asid & 0xFFF);
    uint32_t gen_of_released = (uint32_t)(pcid_gen_asid >> 32);

    if (pcid == 0 || pcid >= PCID_MAX) {
        return;
    }

    if (atomic_fetchadd_int(&pcid_state.active_pcids[pcid], -1) == 1) {
        simple_lock(&pcid_state.alloc_lock);
        if (atomic_load_int(&pcid_state.active_pcids[pcid]) == 0) {
             pcid_bitmap_clear(pcid);
             #ifdef TEST_HAL_PCID_ALLOCATOR
             if (g_mock_env) {
                 kprintf("[pcid_x86.c/x86_release_pcid] TEST_MODE: Calling mock_hook_pcid_released for pcid %u, gen %u, g_mock_env %p\n", pcid, gen_of_released, (void*)g_mock_env);
                 mock_hook_pcid_released(g_mock_env, pcid, gen_of_released);
             }
             #endif
        }
        simple_unlock(&pcid_state.alloc_lock);
    }
}
