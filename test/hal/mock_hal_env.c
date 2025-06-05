#include "mock_hal_env.h"
#include <string.h> // For memset
#include <stdio.h>  // For printf

// Define the actual storage for the global mock environment
mock_hal_env_t g_mock_env_storage;
// Define the global pointer, initially NULL
mock_hal_env_t *g_mock_env = NULL;

void mock_hal_env_init(void) {
    // Point g_mock_env to the actual storage
    g_mock_env = &g_mock_env_storage;

    printf("[mock_hal_env_init] Initializing g_mock_env (pointer %p) to point to storage %p\n",
           (void*)&g_mock_env, (void*)g_mock_env);
    fflush(stdout);

    memset(g_mock_env, 0, sizeof(mock_hal_env_t)); // Clear the storage via the pointer

    // Setup default CPUID values in g_mock_env->cpu.cpuid_results...
    // Basic CPUID Info
    g_mock_env->cpu.cpuid_max_basic_leaf_val = 0xD; // Max basic leaf supported
    g_mock_env->cpu.cpuid_results[0x0][0] = g_mock_env->cpu.cpuid_max_basic_leaf_val; // EAX: Max basic leaf
    g_mock_env->cpu.cpuid_results[0x0][1] = 0x756e6547; // EBX: "Genu"
    g_mock_env->cpu.cpuid_results[0x0][3] = 0x49656e69; // EDX: "ineI"
    g_mock_env->cpu.cpuid_results[0x0][2] = 0x6c65746e; // ECX: "ntel"

    // Feature Information (Leaf 0x1)
    // Simulate PCID support: ECX bit 17 (X86_CR4_PCIDE)
    g_mock_env->cpu.cpuid_results[0x1][2] = X86_CR4_PCIDE; // ECX: Set PCID feature bit

    // Extended CPUID Info
    g_mock_env->cpu.cpuid_max_ext_leaf_val = 0x80000008; // Max extended leaf supported
    // Leaf 0x80000000 provides max extended leaf
    g_mock_env->cpu.cpuid_ext_results[0x0][0] = g_mock_env->cpu.cpuid_max_ext_leaf_val;

    // Leaf 0x80000008: Virtual and Physical Address Sizes
    // EAX: Bits 07-00: Physical Address Bits. Bits 15-08: Linear Address Bits.
    // Index for cpuid_ext_results should be calculated from the leaf number.
    // E.g., 0x80000008 becomes index 8 if MOCK_CPUID_MAX_LEAFS is large enough,
    // or (0x80000008 & (MOCK_CPUID_MAX_LEAFS - 1)) if we are using modulo.
    // The current MOCK_CPUID_MAX_LEAFS is 32.
    // So, 0x80000000 becomes 0, 0x80000001 becomes 1, ..., 0x80000008 becomes 8.
    if ((0x80000008 & (MOCK_CPUID_MAX_LEAFS - 1)) < MOCK_CPUID_MAX_LEAFS) { // Check bounds
         g_mock_env->cpu.cpuid_ext_results[0x80000008 & (MOCK_CPUID_MAX_LEAFS - 1)][0] = (48 << 8) | 48; // 48-bit linear, 48-bit physical
    }


    // Set CR4.PCIDE if CPUID indicates support
    if (g_mock_env->cpu.cpuid_results[0x1][2] & X86_CR4_PCIDE) {
        g_mock_env->cpu.cr4 |= X86_CR4_PCIDE;
    }

    g_mock_env->simulated_cpu_count = 1; // Default to one CPU unless overridden
    g_mock_env->current_simulated_cpu_id = 0;

    printf("[mock_hal_env_init] g_mock_env->cpu.cr4 = 0x%lx after init\n", g_mock_env->cpu.cr4);
    fflush(stdout);
}

void mock_hal_env_reset(void) {
    if (g_mock_env) { // Only if it was initialized (i.e., g_mock_env pointer is not NULL)
        printf("[mock_hal_env_reset] Resetting g_mock_env (pointer %p, storage %p)\n",
               (void*)&g_mock_env, (void*)g_mock_env);
        fflush(stdout);
    } else {
        // This case should ideally not happen if init is always called first.
        // But if it does, initialize first.
        printf("[mock_hal_env_reset] g_mock_env was NULL, calling init first.\n");
        fflush(stdout);
    }
    mock_hal_env_init(); // This will memset and re-init CPUID defaults.
}

/* Mock x86 operations */
void mock_x86_lcr3(mock_hal_env_t *env, uint64_t value) {
    if (!env) return;
    env->cpu.cr3 = value;
    env->metrics.lcr3_count++;
    env->metrics.last_cr3_value = value;
}

uint64_t mock_x86_rcr3(mock_hal_env_t *env) {
    if (!env) return 0;
    return env->cpu.cr3;
}

void mock_x86_lcr4(mock_hal_env_t *env, uint64_t value) {
    if (!env) return;
    bool old_pcide_enabled = (env->cpu.cr4 & X86_CR4_PCIDE) != 0;
    bool new_pcide_enabled = (value & X86_CR4_PCIDE) != 0;

    env->cpu.cr4 = value;
    env->metrics.lcr4_count++;
    env->metrics.last_cr4_value = value;

    if (new_pcide_enabled && !old_pcide_enabled) {
        env->metrics.tlb_flush_count++;
        printf("[mock_x86_lcr4] PCIDE enabled. Global TLB flush implied.\n");
        fflush(stdout);
    } else if (!new_pcide_enabled && old_pcide_enabled) {
        env->metrics.tlb_flush_count++;
        printf("[mock_x86_lcr4] PCIDE disabled. Global TLB flush implied.\n");
        fflush(stdout);
    }
}

uint64_t mock_x86_rcr4(mock_hal_env_t *env) {
    if (!env) return 0;
    return env->cpu.cr4;
}

void mock_x86_cpuid(mock_hal_env_t *env, uint32_t leaf, uint32_t subleaf,
                  uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    if (!env) {
        *eax = *ebx = *ecx = *edx = 0;
        return;
    }

    // subleaf is ignored for this basic mock for now.
    (void)subleaf;

    uint32_t (*source_array)[4];
    uint32_t original_leaf = leaf; // Preserve original leaf for checks against max values
    uint32_t normalized_leaf_for_array = original_leaf; // This will be the index for array access

    if (original_leaf >= 0x80000000) { // Extended CPUID leaf
        source_array = env->cpu.cpuid_ext_results;
        // Normalize the leaf for array indexing (e.g., 0x80000000 maps to 0, 0x80000001 to 1)
        // MOCK_CPUID_MAX_LEAFS is assumed to be a power of 2 for this simple masking.
        normalized_leaf_for_array &= (MOCK_CPUID_MAX_LEAFS - 1);

        if (original_leaf > env->cpu.cpuid_max_ext_leaf_val) {
            *eax = *ebx = *ecx = *edx = 0; // Requested extended leaf is > max supported extended leaf
            return;
        }
        // Further check if the normalized index is within bounds of our array
        if (normalized_leaf_for_array >= MOCK_CPUID_MAX_LEAFS) {
             *eax = *ebx = *ecx = *edx = 0; // Normalized index out of bounds
             return;
        }
    } else { // Basic CPUID leaf
        source_array = env->cpu.cpuid_results;
        // normalized_leaf_for_array is already original_leaf here

        if (original_leaf > env->cpu.cpuid_max_basic_leaf_val) {
            *eax = *ebx = *ecx = *edx = 0; // Requested basic leaf is > max supported basic leaf
            return;
        }
        // Check against actual array bounds for basic leaves
        if (normalized_leaf_for_array >= MOCK_CPUID_MAX_LEAFS) {
            *eax = *ebx = *ecx = *edx = 0; // Leaf index out of bounds for our mock array
            return;
        }
    }

    // If all checks passed, access the array using the normalized_leaf_for_array
    *eax = source_array[normalized_leaf_for_array][0];
    *ebx = source_array[normalized_leaf_for_array][1];
    *ecx = source_array[normalized_leaf_for_array][2];
    *edx = source_array[normalized_leaf_for_array][3];
}

void mock_x86_invlpg(mock_hal_env_t *env, uint64_t va) {
    if (!env) return;
    env->metrics.tlb_flush_count++;
    // printf("[mock_x86_invlpg] TLB entry for VA 0x%lx flushed.\n", va);
}

/* Hooks for pcid_x86.c */
void mock_hook_pcid_allocated(mock_hal_env_t *env, uint32_t pcid, uint32_t gen) {
    if (!env) return;
    env->metrics.pcid_alloc_count++;
    // printf("[mock_hook_pcid_allocated] PCID %u (Gen %u) allocated.\n", pcid, gen);
}

void mock_hook_pcid_released(mock_hal_env_t *env, uint32_t pcid, uint32_t gen) {
    if (!env) return;
    env->metrics.pcid_release_count++;
    // printf("[mock_hook_pcid_released] PCID %u (Gen %u) released.\n", pcid, gen);
}

void mock_hook_pcid_wraparound_flush(mock_hal_env_t *env, uint32_t new_gen) {
    if (!env) return;
    env->metrics.pcid_wraparound_count++;
    env->metrics.tlb_flush_count++;
    printf("[mock_hook_pcid_wraparound_flush] PCID generation wrapped to %u. Global TLB flush implied.\n", new_gen);
    fflush(stdout);
}

/* Main function for testing mock_hal_env.c itself (optional) */
/*
int main() {
    mock_hal_env_init(); // Initialize the global g_mock_env

    printf("Initial CR4: 0x%lx\n", mock_x86_rcr4(g_mock_env));
    printf("Initial Allocations: %u\n", g_mock_env->metrics.pcid_alloc_count);

    mock_hook_pcid_allocated(g_mock_env, 1, 0);
    printf("Allocations after hook: %u\n", g_mock_env->metrics.pcid_alloc_count);

    mock_x86_lcr4(g_mock_env, mock_x86_rcr4(g_mock_env) & ~X86_CR4_PCIDE); // Disable PCIDE
    printf("CR4 after disabling PCIDE: 0x%lx\n", mock_x86_rcr4(g_mock_env));
    printf("TLB flushes: %u\n", g_mock_env->metrics.tlb_flush_count);

    mock_hal_env_reset();
    printf("CR4 after reset: 0x%lx\n", mock_x86_rcr4(g_mock_env));
    printf("Allocations after reset: %u\n", g_mock_env->metrics.pcid_alloc_count);

    return 0;
}
*/
