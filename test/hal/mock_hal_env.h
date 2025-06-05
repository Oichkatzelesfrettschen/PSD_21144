#ifndef TEST_HAL_MOCK_HAL_ENV_H_
#define TEST_HAL_MOCK_HAL_ENV_H_

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define MOCK_CPUID_MAX_LEAFS 32
#define MOCK_CPUID_MAX_SUBLEAFS 4

// Forward declare to handle potential include order issues if used by other stubs
struct mock_hal_env;

typedef struct mock_cpu_state {
    uint64_t cr0;
    uint64_t cr3;
    uint64_t cr4;
    uint32_t cpuid_max_basic_leaf_val;
    uint32_t cpuid_max_ext_leaf_val;
    uint32_t cpuid_results[MOCK_CPUID_MAX_LEAFS][4];
    uint32_t cpuid_ext_results[MOCK_CPUID_MAX_LEAFS][4]; // For 0x8xxxxxxx leaves
} mock_cpu_state_t;

typedef struct mock_metrics {
    uint32_t lcr3_count;
    uint32_t lcr4_count;
    uint32_t tlb_flush_count; // For invlpg or equivalent full flushes
    uint32_t pcid_alloc_count;
    uint32_t pcid_release_count;
    uint32_t pcid_wraparound_count; // Incremented when PCID generation increases
    uint64_t last_cr3_value;
    uint64_t last_cr4_value;
} mock_metrics_t;

typedef struct mock_hal_env {
    mock_cpu_state_t cpu;
    mock_metrics_t metrics;
    bool force_pcid_exhaustion; // Test flag to simulate all PCIDs being in use
    uint32_t fail_at_allocation_n; // Test flag to make Nth allocation attempt fail
    uint32_t current_alloc_attempt; // Counter for fail_at_allocation_n
    uint32_t simulated_cpu_count;
    uint32_t current_simulated_cpu_id;
    // void (*invariant_checker)(struct mock_hal_env*);
    // void (*state_dumper)(struct mock_hal_env*);
} mock_hal_env_t;

// Declarations for the global environment
extern mock_hal_env_t g_mock_env_storage; // Declaration of the actual storage
extern mock_hal_env_t *g_mock_env;       // Declaration of the pointer

/* Initialize/Reset the global mock environment */
void mock_hal_env_init(void);
void mock_hal_env_reset(void); // New declaration

/* Mock x86 operations */
void mock_x86_lcr3(mock_hal_env_t *env, uint64_t value);
uint64_t mock_x86_rcr3(mock_hal_env_t *env);
void mock_x86_lcr4(mock_hal_env_t *env, uint64_t value);
uint64_t mock_x86_rcr4(mock_hal_env_t *env);
void mock_x86_cpuid(mock_hal_env_t *env, uint32_t leaf, uint32_t subleaf,
                  uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);
void mock_x86_invlpg(mock_hal_env_t *env, uint64_t va);

/* Hooks for pcid_x86.c */
void mock_hook_pcid_allocated(mock_hal_env_t *env, uint32_t pcid, uint32_t gen);
void mock_hook_pcid_released(mock_hal_env_t *env, uint32_t pcid, uint32_t gen);
void mock_hook_pcid_wraparound_flush(mock_hal_env_t *env, uint32_t new_gen);

/* Verification helpers */
static inline void verify_pcid_in_cr3(uint64_t cr3_val) {
    uint32_t pcid = cr3_val & 0xFFF;
    assert(pcid < 4096); // PCID is 12 bits
    // Ensure base address part of CR3 is page-aligned (lowest 12 bits zero)
    // This is a property of the page directory base address, not directly PCID related,
    // but good to check if CR3 is formed correctly.
    assert((cr3_val & ~0xFFFULL) % 4096 == 0);
}

#ifndef X86_CR4_PCIDE // Centralize definition for test code
#define X86_CR4_PCIDE (1 << 17)
#endif

#endif /* TEST_HAL_MOCK_HAL_ENV_H_ */
