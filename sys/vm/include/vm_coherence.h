#ifndef _SYS_VM_COHERENCE_H_
#define _SYS_VM_COHERENCE_H_

// Decide on includes based on standalone mode for testing
#if defined(STANDALONE_INTEGRATION_TEST) || defined(VM_SEMANTIC_FSM_STANDALONE) || defined(VM_SEMANTIC_VALIDATORS_STANDALONE) || defined(VM_SEMANTIC_NOTIFY_STANDALONE) || defined(VM_COHERENCE_STANDALONE) || defined(VM_TEST_SEMANTIC_TRANSITIONS_STANDALONE)
// Standalone Mode: Include standard headers for base types
#include <stdint.h> // For uintN_t, intN_t
#include <stddef.h> // For size_t
#include <stdbool.h> // For bool (though often defined as int in kernel contexts)

typedef uint64_t vm_paddr_t; // Physical address
typedef uint64_t vm_offset_t; // Virtual memory offset or address
typedef int vm_prot_t;      // Memory protection
typedef size_t vsize_t;     // For region sizes (using standard size_t)

#define VM_PROT_READ    0x01
#define VM_PROT_WRITE   0x02
#define VM_PROT_EXECUTE 0x04

// Error codes (typically from errno.h or similar in a full system)
#define EINVAL 22
#define EBUSY 16
#define EPERM 1
#define EAGAIN 35
#define ENOMEM 12

// Minimal stub for rwlock_t if not provided by system headers in this mode
typedef struct { volatile intlock_t lock; } rwlock_t; // Simplified
// Minimal proc struct for function signatures
struct proc { int p_pid; };


#else // Kernel Mode: Include kernel-specific headers
#include <sys/types.h> // Kernel's types (pulls in stdint if configured, or its own)
#include <sys/lock.h>  // Kernel's rwlock_t
// vm_offset_t, vm_paddr_t etc. would be defined by sys/types.h or machine/types.h
#endif // End of standalone vs kernel specific includes/typedefs

// Common includes for both modes (vm_semantic.h needs careful review for its own includes)
#include "vm_semantic.h" // For struct semantic_descriptor

#define MAX_VIRT_LEVELS 4 // Example

struct level_mapping {
    int lm_level;
    vm_offset_t lm_vaddr;
    vm_prot_t lm_prot;
    int lm_active; // boolean
    uint64_t lm_generation;
};

struct coherence_state {
    // Fields needed by vm_semantic_fsm.c
    rwlock_t cs_lock;
    struct semantic_descriptor cs_semantic; // Canonical semantic state for this region

#if defined(STANDALONE_INTEGRATION_TEST) || defined(VM_SEMANTIC_FSM_STANDALONE) || defined(VM_SEMANTIC_VALIDATORS_STANDALONE) || defined(VM_SEMANTIC_NOTIFY_STANDALONE) || defined(VM_COHERENCE_STANDALONE) || defined(VM_TEST_SEMANTIC_TRANSITIONS_STANDALONE)
    vm_offset_t cs_phys_base_stub; // For stubbing physical address
    vsize_t cs_size_stub;          // For stubbing size (using standard size_t via vsize_t)
#else
    // Kernel specific fields for actual addresses if different from descriptor's conceptual view
    vm_offset_t cs_kernel_view_start_addr;
    vm_offset_t cs_kernel_view_end_addr;
#endif
    uint64_t cs_write_generation;
    int cs_active_writers; // Number of active writers to the region
    int cs_distributed;    // Boolean: is this region part of distributed coherence?
    struct level_mapping cs_mappings[MAX_VIRT_LEVELS]; // Mappings at different virt levels

    // Other fields for coherence management (e.g., physical page list, object pointer)
    // TAILQ_ENTRY(coherence_state) cs_link; // If part of a list
};

// Stub for vm_coherence_dist.h contents if not creating that file yet
// This is included by vm_semantic_fsm.c when VM_SEMANTIC_FSM_STANDALONE is defined
// So, the struct vector_clock should be here.
#ifndef _SYS_VM_COHERENCE_DIST_H_
#define _SYS_VM_COHERENCE_DIST_H_

// Forward declare if not defined, or ensure it's defined before use by vm_semantic_fsm.c
// struct coherence_state; // vm_semantic_fsm.c defines it if standalone if this header is included by it.
                           // vm_semantic_notify.c also defines coherence_state for its standalone build.

struct vector_clock { // Ensure this is defined for FSM and Notify standalone
    int dummy;
};

// For vm_semantic_notify.c standalone test
struct remote_node {
    int rn_node_id;
    struct remote_node *rn_next;
};

struct dist_coherence_state {
    // struct coherence_state dcs_local; // No longer an issue as coh is defined before this
    // other distributed fields
#if defined(STANDALONE_INTEGRATION_TEST) || defined(VM_SEMANTIC_FSM_STANDALONE) || defined(VM_SEMANTIC_VALIDATORS_STANDALONE) || defined(VM_SEMANTIC_NOTIFY_STANDALONE) || defined(VM_COHERENCE_STANDALONE) || defined(VM_TEST_SEMANTIC_TRANSITIONS_STANDALONE)
    struct remote_node *dcs_sharers_stub_if_distributed; // Used by notify standalone test
#else
    // struct TAILQ_HEAD(, remote_node) dcs_sharers; // Example for kernel list
#endif
};
#endif // _SYS_VM_COHERENCE_DIST_H_


// Functions used by FSM (stubs if not fully implemented)
void vm_coherence_init(void); // General initialization for the coherence system
int vm_coherence_establish(vm_paddr_t phys_base, size_t size, struct semantic_descriptor *initial_sem_desc, struct coherence_state **state_out);
int vm_coherence_map(struct coherence_state *state, int level, vm_offset_t vaddr, vm_prot_t prot); // Map a view of the coherent region
void vm_coherence_unmap(struct coherence_state *state, int level, vm_offset_t vaddr); // Unmap a view
void vm_coherence_destroy(struct coherence_state *coh); // Destroy and free coherence state
void vm_coherence_mark_write(struct coherence_state *coh, int level); // Mark that a write occurred at a specific level
void vm_coherence_mark_read(struct coherence_state *coh, int level);  // Mark that a read occurred (if needed for coherence protocols)

#endif /* _SYS_VM_COHERENCE_H_ */
