#ifndef PCID_X86_TEST_STUBS_H_
#define PCID_X86_TEST_STUBS_H_

#ifndef PCID_MAX // Allow override if defined elsewhere, though unlikely for this stub header
#define PCID_MAX 4096
#endif

// --- Standard C Headers ---
#include <stdarg.h> // MUST BE EARLY for __gnuc_va_list used by stdio.h
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// --- Block Conflicting Kernel Headers ---
// Define guards for headers that pcid_x86.c might try to include in its non-test path.
// This prevents them from being processed if -I paths accidentally find them.
#define _SYS_TYPES_H_
#define _SYS_PARAM_H_
#define _SYS_PROC_H_
#define _SYS_SYSTM_H_
#define _SYS_LOCK_H_
#define _SYS_ATOMIC_H_       // Block kernel's general atomic header
#define _MACHINE_ATOMIC_H_   // Block kernel's machine-specific atomic header
#define _MACHINE_X86_MMU_UTILS_H_ // For x86_rcr4, x86_lcr4 (will be mocked)
#define _MACHINE_SPECIALREG_H_    // For CR4_PCIDE (will be defined locally)


// --- Minimal Kernel Type Definitions & Stubs ---
typedef uint64_t uintptr_t; // Common assumption for addresses/pointers in kernel code
// Standard integer types (uint32_t, etc.) are included via <stdint.h>
typedef unsigned int u_int; // Common BSD alias

struct proc {
    int p_pid_stub; // Minimal if pcid_x86.c needs to dereference for debug, else not needed
    // Add p_arch_asid and p_paddr_pgdir if pcid_x86.c itself (not just tests) needs to dereference them.
    // Currently, x86_allocate_pcid takes struct proc* but doesn't use its members.
    // hal_i386.c (caller of x86_allocate_pcid) uses p->p_arch_asid.
    uint64_t p_arch_asid;
    uintptr_t p_paddr_pgdir;
};

// Minimal stub for lock_object and its functions
typedef struct { char lock_data_stub[32]; /* Minimal stub */ } lock_object_stub_t;
#define simple_lock_init(lock_ptr, name)         do { /* printf("STUB_LOCK: init '%s'\n", name); */ (void)name; memset((void*)(lock_ptr), 0, sizeof(lock_object_stub_t)); /* Basic init */        } while(0)
#define simple_lock(lock_ptr)                    do { /* printf("STUB_LOCK: acquire %s\n", ((lock_object_stub_t*)(lock_ptr))->lock_data_stub); */ (void)lock_ptr; } while(0)
#define simple_unlock(lock_ptr)                  do { /* printf("STUB_LOCK: release %s\n", ((lock_object_stub_t*)(lock_ptr))->lock_data_stub); */ (void)lock_ptr; } while(0)

#undef kprintf
#define kprintf(...) do { printf(__VA_ARGS__); fflush(stdout); } while(0)
#undef panic
#define panic(msg_panic)        do {            fprintf(stderr, "PANIC_STUB: %s (file %s, line %d)\n", msg_panic, __FILE__, __LINE__); fflush(stderr); assert(0); abort();        } while(0)

// --- Definitions needed from blocked headers ---
#ifndef CR4_PCIDE // If specialreg.h was blocked
#define CR4_PCIDE (1 << 17)
#endif
// Define X86_CR4_PCIDE as well, as mock_hal_env.h uses it, and pcid_x86.c might too via redirection
#ifndef X86_CR4_PCIDE
#define X86_CR4_PCIDE CR4_PCIDE // Alias for tests, mock_hal_env.h also defines X86_CR4_PCIDE
#endif


// --- Mocked Hardware Access (redirects to mock_hal_env) ---
#include "mock_hal_env.h" // Provides g_mock_env and mock_x86_* declarations

#define x86_rcr4() mock_x86_rcr4(g_mock_env)
#define x86_lcr4(val) mock_x86_lcr4(g_mock_env, val)
// Add x86_rcr3 if perform_global_tlb_flush_locked uses it (it did not in last version)
// #define x86_rcr3() mock_x86_rcr3(g_mock_env)
// #define x86_lcr3(val) mock_x86_lcr3(g_mock_env, val)


// --- Atomic Operations (using GCC/Clang builtins for tests) ---
#if defined(__GNUC__) || defined(__clang__)
    #define atomic_load_acq_64(ptr) __atomic_load_n((ptr), __ATOMIC_ACQUIRE)
    #define atomic_store_rel_64(ptr, val) __atomic_store_n((ptr), val, __ATOMIC_RELEASE) // Not used by pcid_x86.c

    // For atomic_cmpset_64(ptr, oldval, newval) returning bool (true on success)
    #define atomic_cmpset_64(ptr, oldval, newval) __sync_bool_compare_and_swap((volatile uint64_t*)ptr, oldval, newval)

    #define atomic_store_int(ptr, val) __atomic_store_n((volatile unsigned int*)ptr, val, __ATOMIC_RELAXED) // Or __ATOMIC_RELEASE
    #define atomic_load_int(ptr) __atomic_load_n((volatile unsigned int*)ptr, __ATOMIC_RELAXED) // Or __ATOMIC_ACQUIRE

    // atomic_fetchadd_int used by pcid_x86.c returns old value, __sync_fetch_and_add does this.
    #define atomic_fetchadd_int(ptr, val) __sync_fetch_and_add((volatile unsigned int*)ptr, val)

    // atomic_dec_int_nv not directly available, use __sync_sub_and_fetch for "decrement and fetch new value"
    // pcid_x86.c uses `if (atomic_fetchadd_int(ptr, -1) == 1)` which means "if value was 1 before decrement"
    // This is fine with atomic_fetchadd_int as defined above.

#else
    #warning "Using non-atomic stubs for atomic operations. Concurrent tests will not be meaningful."
    // Provide non-atomic fallbacks that will fail concurrent tests but allow single-threaded logic to be tested.
    #define atomic_load_acq_64(ptr) (*(ptr))
    #define atomic_store_rel_64(ptr, val) (*(ptr) = val)
    static inline bool test_atomic_cas_u64_stub(volatile uint64_t *ptr, uint64_t expected, uint64_t desired) {
        if (*ptr == expected) { *ptr = desired; return true; }
        return false;
    }
    #define atomic_cmpset_64(ptr, oldval, newval) test_atomic_cas_u64_stub(ptr, oldval, newval)
    #define atomic_store_int(ptr, val) ((*(ptr)) = (val), (void)0) // void to avoid warnings on assignment result
    #define atomic_load_int(ptr) (*(ptr))
    static inline unsigned int test_atomic_fetchadd_int_stub(volatile unsigned int* ptr, int val) { unsigned int old = *ptr; *ptr += val; return old;}
    #define atomic_fetchadd_int(ptr, val) test_atomic_fetchadd_int_stub(ptr, val)
#endif

#endif /* PCID_X86_TEST_STUBS_H_ */
