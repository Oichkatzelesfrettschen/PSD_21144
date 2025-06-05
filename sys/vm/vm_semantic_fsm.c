/*
 * Semantic Finite State Machine
 *
 * This module manages transitions between semantic domains. Each
 * transition must preserve memory coherence and maintain invariants
 * across all virtualization levels and network nodes.
 */

#ifdef VM_SEMANTIC_FSM_STANDALONE
#include <stdio.h>   // For printf
#include <stdlib.h>  // For malloc, free, NULL
#include <string.h>  // For memset
#include <assert.h>  // For assert() in tests
#include <stdint.h>  // For C99 integer types (e.g. uintptr_t, uint64_t)
#include <stddef.h>  // For size_t
#include <stdbool.h> // For bool, true, false

// Types like vm_offset_t, size_t, vm_prot_t, struct proc, error codes (EINVAL etc.),
// VM_PROT_READ etc. are now expected to be defined in vm_coherence.h or vm_semantic.h
// when VM_SEMANTIC_FSM_STANDALONE is defined.

// Stubs for semantic and coherence structures/functions
#include "include/vm_semantic.h" // Provides struct semantic_descriptor, enum semantic_domain, SEM_ATTR_*, MESSAGE_MAGIC, struct message_header
#include "include/vm_coherence.h"// Provides struct coherence_state, vm_offset_t, size_t, vm_prot_t, struct proc, error codes, VM_PROT_*, struct vector_clock (stub)

// vm_semantic_context definition (already present from previous merge)
// Ensure it uses types now available from headers (e.g. size_t from stddef.h via vm_coherence.h)
// struct vm_semantic_context { ... }; // This is already in the file from the previous merge.

// Use local static inline for rw_* stubs to avoid linking issues if other modules define them differently for their own tests.
static inline void fsm_rw_init(void *l, const char *n) { (void)l; (void)n; printf("FSM_STUB: rw_init for %s\n", n); }
static inline void fsm_rw_wlock(void *l) { (void)l; printf("FSM_STUB: rw_wlock\n"); }
static inline void fsm_rw_wunlock(void *l) { (void)l; printf("FSM_STUB: rw_wunlock\n"); }
// static inline void fsm_rw_rlock(void *l) { (void)l; } // Not used by current FSM logic
// static inline void fsm_rw_runlock(void *l) { (void)l; } // Not used by current FSM logic

// These are now defined in vm_semantic_notify.c for standalone, and declared for kernel below
// static void notify_transition_all_levels(struct coherence_state *coh, struct vm_semantic_context *ctx) {
//     printf("Notification: Transition for domain %d to %d\n", ctx->vsc_old_domain, ctx->vsc_new_domain);
// }
// static void propagate_transition_network(struct coherence_state *coh, struct vm_semantic_context *ctx) {
//     printf("Propagation: Transition for domain %d to %d\n", ctx->vsc_old_domain, ctx->vsc_new_domain);
// }

// Placeholder for validator and action functions (enhance these for the tests)
static int validate_data_to_message(struct vm_semantic_context *ctx) {
    printf("FSM_STUB_VALIDATOR: DATA -> MESSAGE for addr 0x%lx, size %zu\n", (unsigned long)ctx->vsc_addr, ctx->vsc_size);
    if (ctx && ctx->vsc_addr) {
        struct message_header *hdr = (struct message_header *)ctx->vsc_addr;
        if (ctx->vsc_size < sizeof(struct message_header)) {
             printf("FSM_STUB_VALIDATOR: Validation failed - region too small for header.\n");
            return EINVAL;
        }
        if (hdr->mh_magic != MESSAGE_MAGIC) {
             printf("FSM_STUB_VALIDATOR: Validation failed - wrong message magic %x vs %x.\n", hdr->mh_magic, MESSAGE_MAGIC);
            return EINVAL;
        }
        if ((sizeof(struct message_header) + hdr->mh_size) > ctx->vsc_size) {
             printf("FSM_STUB_VALIDATOR: Validation failed - message payload size %u exceeds region %zu (header %zu)\n", hdr->mh_size, ctx->vsc_size, sizeof(struct message_header));
            return EINVAL;
        }
    }
    printf("FSM_STUB_VALIDATOR: DATA -> MESSAGE PASSED\n");
    return 0;
}
static void action_data_to_message(struct vm_semantic_context *ctx) {
    printf("FSM_STUB_ACTION: DATA -> MESSAGE. Setting attributes.\n");
    if (ctx && ctx->vsc_coherence) {
        ctx->vsc_coherence->cs_semantic.attributes |= SEM_ATTR_IN_TRANSIT;
        ctx->vsc_coherence->cs_semantic.attributes &= ~SEM_ATTR_BUFFER_VALID;
    }
}

static int validate_message_to_data(struct vm_semantic_context *ctx) {
    printf("FSM_STUB_VALIDATOR: MESSAGE -> DATA\n");
    return 0;
}
static void action_message_to_data(struct vm_semantic_context *ctx) {
    printf("FSM_STUB_ACTION: MESSAGE -> DATA. Setting attributes.\n");
    if (ctx && ctx->vsc_coherence) {
        ctx->vsc_coherence->cs_semantic.attributes &= ~(SEM_ATTR_IN_TRANSIT | SEM_ATTR_DELIVERED);
        ctx->vsc_coherence->cs_semantic.attributes |= SEM_ATTR_BUFFER_VALID;
    }
}

static int validate_heap_to_text(struct vm_semantic_context *ctx) {
    printf("FSM_STUB_VALIDATOR: HEAP -> TEXT. Attrs: 0x%x\n", ctx->vsc_coherence->cs_semantic.attributes);
    if (ctx && ctx->vsc_coherence) {
        if (!(ctx->vsc_coherence->cs_semantic.attributes & SEM_ATTR_COMPILED)) {
            printf("FSM_STUB_VALIDATOR: HEAP -> TEXT failed, SEM_ATTR_COMPILED missing.\n");
            return EPERM;
        }
    }
    return 0;
}
static void action_heap_to_text(struct vm_semantic_context *ctx) {
    printf("FSM_STUB_ACTION: HEAP -> TEXT. Setting attributes.\n");
    if (ctx && ctx->vsc_coherence) {
        ctx->vsc_coherence->cs_semantic.attributes &= ~SEM_ATTR_WRITABLE;
        ctx->vsc_coherence->cs_semantic.attributes |= SEM_ATTR_EXECUTABLE;
    }
}

static int validate_matrix_to_message(struct vm_semantic_context *ctx) {
    printf("FSM_STUB_VALIDATOR: MATRIX -> MESSAGE\n");
    return 0;
}
static void action_matrix_to_message(struct vm_semantic_context *ctx) {
    printf("FSM_STUB_ACTION: MATRIX -> MESSAGE. Setting attributes.\n");
    if (ctx && ctx->vsc_coherence) {
        ctx->vsc_coherence->cs_semantic.attributes |= SEM_ATTR_IN_TRANSIT;
    }
}

// Test functions moved from test_semantic_transitions.c
static void setup_test_coherence_state(
    struct coherence_state *coh,
    enum semantic_domain initial_domain,
    uint32_t initial_attributes,
    vm_offset_t base_addr_stub,
    size_t size_stub) // size_t here should be from stddef.h
{
    memset(coh, 0, sizeof(struct coherence_state));
    fsm_rw_init(&coh->cs_lock, "test_coh_lock"); // Use FSM-local stub
    coh->cs_semantic.domain = initial_domain;
    coh->cs_semantic.attributes = initial_attributes;
    coh->cs_semantic.version = 1;
    coh->cs_semantic.transition_count = 0;
    coh->cs_phys_base_stub = base_addr_stub; // These are specific to VM_SEMANTIC_FSM_STANDALONE
    coh->cs_size_stub = size_stub;
    coh->cs_write_generation = 0;
    coh->cs_active_writers = 0;
    coh->cs_distributed = 0;
}

struct proc test_proc = { .p_pid = 123 };

void test_jit_compilation_transition() {
    printf("--- Test: JIT Compilation (HEAP -> TEXT) ---\n");
    struct coherence_state coh;
    setup_test_coherence_state(&coh, SEM_HEAP, SEM_ATTR_WRITABLE | SEM_ATTR_COMPILED, 0x1000, 0x1000);

    printf("Initial: Domain=%d, Attrs=0x%x\n", coh.cs_semantic.domain, coh.cs_semantic.attributes);
    int result = vm_semantic_transition(&coh, SEM_TEXT, &test_proc);
    printf("Transition result: %d\n", result);
    assert(result == 0);
    assert(coh.cs_semantic.domain == SEM_TEXT);
    assert(coh.cs_semantic.transition_count == 1);
    assert(coh.cs_write_generation == 1);
    assert(!(coh.cs_semantic.attributes & SEM_ATTR_WRITABLE));
    assert(coh.cs_semantic.attributes & SEM_ATTR_EXECUTABLE);
    assert(coh.cs_semantic.attributes & SEM_ATTR_COMPILED);
    printf("Final: Domain=%d, Attrs=0x%x, Gen=%llu, Count=%u\n",
           coh.cs_semantic.domain, coh.cs_semantic.attributes,
           (unsigned long long)coh.cs_write_generation, coh.cs_semantic.transition_count);
    printf("JIT Compilation Test PASSED\n\n");
}

void test_ipc_send_transition() {
    printf("--- Test: IPC Send (DATA -> MESSAGE) ---\n");
    struct coherence_state coh;
    size_t buffer_size = sizeof(struct message_header) + 10;
    char *buffer = malloc(buffer_size);
    assert(buffer != NULL);
    struct message_header *hdr = (struct message_header *)buffer;

    hdr->mh_magic = MESSAGE_MAGIC;
    hdr->mh_size = 5;

    setup_test_coherence_state(&coh, SEM_DATA, SEM_ATTR_WRITABLE, (vm_offset_t)(uintptr_t)buffer, buffer_size);

    printf("Initial: Domain=%d, Attrs=0x%x\n", coh.cs_semantic.domain, coh.cs_semantic.attributes);
    int result = vm_semantic_transition(&coh, SEM_MESSAGE, &test_proc);
    printf("Transition result: %d\n", result);
    assert(result == 0);
    assert(coh.cs_semantic.domain == SEM_MESSAGE);
    assert(coh.cs_semantic.transition_count == 1);
    assert(coh.cs_write_generation == 1);
    assert(coh.cs_semantic.attributes & SEM_ATTR_IN_TRANSIT);
    printf("Final: Domain=%d, Attrs=0x%x, Gen=%llu, Count=%u\n",
           coh.cs_semantic.domain, coh.cs_semantic.attributes,
           (unsigned long long)coh.cs_write_generation, coh.cs_semantic.transition_count);
    free(buffer);
    printf("IPC Send Test PASSED\n\n");
}

void test_ipc_receive_transition() {
    printf("--- Test: IPC Receive (MESSAGE -> DATA) ---\n");
    struct coherence_state coh;
    setup_test_coherence_state(&coh, SEM_MESSAGE, SEM_ATTR_IN_TRANSIT | SEM_ATTR_DELIVERED, 0x3000, 0x1000);

    printf("Initial: Domain=%d, Attrs=0x%x\n", coh.cs_semantic.domain, coh.cs_semantic.attributes);
    int result = vm_semantic_transition(&coh, SEM_DATA, &test_proc);
    printf("Transition result: %d\n", result);
    assert(result == 0);
    assert(coh.cs_semantic.domain == SEM_DATA);
    assert(coh.cs_semantic.transition_count == 1);
    assert(coh.cs_write_generation == 1);
    assert(!(coh.cs_semantic.attributes & SEM_ATTR_IN_TRANSIT));
    assert(!(coh.cs_semantic.attributes & SEM_ATTR_DELIVERED));
    assert(coh.cs_semantic.attributes & SEM_ATTR_BUFFER_VALID);
    printf("Final: Domain=%d, Attrs=0x%x, Gen=%llu, Count=%u\n",
           coh.cs_semantic.domain, coh.cs_semantic.attributes,
           (unsigned long long)coh.cs_write_generation, coh.cs_semantic.transition_count);
    printf("IPC Receive Test PASSED\n\n");
}

void test_matrix_to_message_transition() {
    printf("--- Test: Matrix to Message (MATRIX -> MESSAGE) ---\n");
    struct coherence_state coh;
    setup_test_coherence_state(&coh, SEM_MATRIX, 0, 0x4000, 0x2000);

    printf("Initial: Domain=%d, Attrs=0x%x\n", coh.cs_semantic.domain, coh.cs_semantic.attributes);
    int result = vm_semantic_transition(&coh, SEM_MESSAGE, &test_proc);
    printf("Transition result: %d\n", result);
    assert(result == 0);
    assert(coh.cs_semantic.domain == SEM_MESSAGE);
    assert(coh.cs_semantic.transition_count == 1);
    assert(coh.cs_write_generation == 1);
    assert(coh.cs_semantic.attributes & SEM_ATTR_IN_TRANSIT);
    printf("Final: Domain=%d, Attrs=0x%x, Gen=%llu, Count=%u\n",
           coh.cs_semantic.domain, coh.cs_semantic.attributes,
           (unsigned long long)coh.cs_write_generation, coh.cs_semantic.transition_count);
    printf("Matrix to Message Test PASSED\n\n");
}

void test_invalid_transition_no_rule() {
    printf("--- Test: Invalid Transition (No Rule SEM_TEXT -> SEM_HEAP) ---\n");
    struct coherence_state coh;
    setup_test_coherence_state(&coh, SEM_TEXT, SEM_ATTR_EXECUTABLE, 0x5000, 0x1000);

    printf("Initial: Domain=%d, Attrs=0x%x\n", coh.cs_semantic.domain, coh.cs_semantic.attributes);
    int result = vm_semantic_transition(&coh, SEM_HEAP, &test_proc);
    printf("Transition result: %d (Expected EINVAL %d)\n", result, EINVAL);
    assert(result == EINVAL);
    assert(coh.cs_semantic.domain == SEM_TEXT);
    assert(coh.cs_semantic.transition_count == 0);
    printf("Invalid Transition (No Rule) Test PASSED\n\n");
}

void test_invalid_transition_forbidden_attr() {
    printf("--- Test: Invalid Transition (Forbidden Attr SEM_DATA -> SEM_MESSAGE with EXECUTING) ---\n");
    struct coherence_state coh;
    setup_test_coherence_state(&coh, SEM_DATA, SEM_ATTR_WRITABLE | SEM_ATTR_EXECUTING, 0x6000, 0x1000);

    printf("Initial: Domain=%d, Attrs=0x%x\n", coh.cs_semantic.domain, coh.cs_semantic.attributes);
    int result = vm_semantic_transition(&coh, SEM_MESSAGE, &test_proc);
    printf("Transition result: %d (Expected EBUSY %d)\n", result, EBUSY);
    assert(result == EBUSY);
    assert(coh.cs_semantic.domain == SEM_DATA);
    assert(coh.cs_semantic.transition_count == 0);
    printf("Invalid Transition (Forbidden Attr) Test PASSED\n\n");
}

void test_invalid_transition_required_attr_missing() {
    printf("--- Test: Invalid Transition (Required Attr Missing HEAP -> TEXT without COMPILED) ---\n");
    struct coherence_state coh;
    setup_test_coherence_state(&coh, SEM_HEAP, SEM_ATTR_WRITABLE, 0x7000, 0x1000);

    printf("Initial: Domain=%d, Attrs=0x%x\n", coh.cs_semantic.domain, coh.cs_semantic.attributes);
    int result = vm_semantic_transition(&coh, SEM_TEXT, &test_proc);
    printf("Transition result: %d (Expected EPERM %d)\n", result, EPERM);
    assert(result == EPERM);
    assert(coh.cs_semantic.domain == SEM_HEAP);
    assert(coh.cs_semantic.transition_count == 0);
    printf("Invalid Transition (Required Attr Missing) Test PASSED\n\n");
}
// --- End of moved test functions ---

#else
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/malloc.h> // For M_VMSEM, M_WAITOK, M_ZERO if not standalone

#include <vm/include/vm.h> // General VM includes
#include <vm/include/vm_semantic.h>
#include <vm/include/vm_coherence.h>
#include <vm/include/vm_coherence_dist.h> // For dist_coherence_state if used directly

// Forward declare for now, will be in separate headers
struct vm_semantic_context; // This is defined in this file for standalone, but might be in a header for kernel

// Notification and propagation functions (actual definitions in vm_semantic_notify.c)
void notify_transition_all_levels(struct coherence_state *coh, struct vm_semantic_context *ctx);
void propagate_transition_network(struct coherence_state *coh, struct vm_semantic_context *ctx);

// Actual functions will be in vm_semantic_validators.c
int validate_data_to_message(struct vm_semantic_context *ctx);
void action_data_to_message(struct vm_semantic_context *ctx);
int validate_message_to_data(struct vm_semantic_context *ctx);
void action_message_to_data(struct vm_semantic_context *ctx);
int validate_heap_to_text(struct vm_semantic_context *ctx);
void action_heap_to_text(struct vm_semantic_context *ctx);
int validate_matrix_to_message(struct vm_semantic_context *ctx);
void action_matrix_to_message(struct vm_semantic_context *ctx);

#endif


/*
 * Semantic transition rules define valid state changes
 */
struct semantic_transition {
    enum semantic_domain st_from;      /* Source domain */
    enum semantic_domain st_to;        /* Destination domain */
    uint32_t st_required_attrs;        /* Required attributes for 'from' domain */
    uint32_t st_forbidden_attrs;       /* Forbidden attributes for 'from' domain */
    int (*st_validator)(struct vm_semantic_context *ctx); /* Custom validation */
    void (*st_action)(struct vm_semantic_context *ctx);   /* Transition action */
};

/* Define the valid transitions */
// Note: SEM_TEXT is used as per existing vm_semantic.c instead of SEM_CODE
static struct semantic_transition valid_transitions[] = {
    /* Data can become a message when queued for IPC */
    {
        .st_from = SEM_DATA,
        .st_to = SEM_MESSAGE,
        .st_required_attrs = 0, // e.g., SEM_ATTR_BUFFER_VALID if needed
        .st_forbidden_attrs = SEM_ATTR_EXECUTING | SEM_ATTR_COMPUTING,
        .st_validator = validate_data_to_message,
        .st_action = action_data_to_message
    },
    /* Message becomes data after delivery */
    {
        .st_from = SEM_MESSAGE,
        .st_to = SEM_DATA,
        .st_required_attrs = SEM_ATTR_DELIVERED,
        .st_forbidden_attrs = SEM_ATTR_IN_TRANSIT,
        .st_validator = validate_message_to_data,
        .st_action = action_message_to_data
    },
    /* Heap can become text (executable code) through JIT compilation */
    {
        .st_from = SEM_HEAP, // Assuming SEM_HEAP is defined or will be, otherwise use SEM_DATA
        .st_to = SEM_TEXT,
        .st_required_attrs = SEM_ATTR_COMPILED,
        .st_forbidden_attrs = SEM_ATTR_WRITABLE, // Code should not be writable after JIT
        .st_validator = validate_heap_to_text,
        .st_action = action_heap_to_text
    },
    /* Matrix can become message for network transmission */
    {
        .st_from = SEM_MATRIX,
        .st_to = SEM_MESSAGE,
        .st_required_attrs = 0,
        .st_forbidden_attrs = SEM_ATTR_COMPUTING,
        .st_validator = validate_matrix_to_message,
        .st_action = action_matrix_to_message
    },
    // Add other transitions like SEM_MESSAGE to SEM_MATRIX if applicable
    /* Sentinel */
    { .st_from = SEM_NONE, .st_to = SEM_NONE } // SEM_NONE indicates end of list
};

static struct semantic_transition *
find_transition(enum semantic_domain from, enum semantic_domain to)
{
    for (int i = 0; valid_transitions[i].st_from != SEM_NONE; ++i) {
        if (valid_transitions[i].st_from == from && valid_transitions[i].st_to == to) {
            return &valid_transitions[i];
        }
    }
    return NULL;
}

/*
 * Attempt a semantic transition
 *
 * This is the core function that manages domain transitions. It ensures
 * all invariants are maintained and all observers are notified.
 */
int
vm_semantic_transition(struct coherence_state *coh,
                      enum semantic_domain new_domain,
                      struct proc *p) // proc can be NULL if transition is kernel-internal
{
    struct vm_semantic_context ctx;
    struct semantic_transition *trans_rule;
    struct semantic_descriptor *current_desc;
    int error;

    if (coh == NULL) {
        return EINVAL; // Coherence state is essential
    }
    // In a real kernel, you'd get the descriptor from the coherence state.
    // For standalone, we might need to pass it or have the coherence state stub include it.
    // After vm_coherence.h change, cs_semantic is always direct.
    current_desc = &coh->cs_semantic;

    // Ensure the domain is initialized if that's a concern.
    // For now, assume vm_coherence_establish handles this.
    // if (current_desc->domain == SEM_NONE && ctx->vsc_old_domain != SEM_NONE) {
    //     return EINVAL; // Or some other error indicating uninitialized semantic part
    // }

    if (current_desc->domain == new_domain) {
        return 0; // No transition needed
    }

    /* Find the matching transition rule */
    trans_rule = find_transition(current_desc->domain, new_domain);
    if (trans_rule == NULL) {
        return EINVAL;  /* Invalid or undefined transition */
    }

    /* Populate the context for validators and actions */
    memset(&ctx, 0, sizeof(ctx)); // Initialize context
    ctx.vsc_coherence = coh;
    ctx.vsc_proc = p;
#ifdef VM_SEMANTIC_FSM_STANDALONE
    ctx.vsc_addr = coh->cs_phys_base_stub; // Use stub field
    ctx.vsc_size = coh->cs_size_stub;     // Use stub field
#else
    // In kernel, these might come from the coherence state or be passed if more dynamic
    // These fields (cs_kernel_view_start_addr, etc.) are now in vm_coherence.h
    ctx.vsc_addr = coh->cs_kernel_view_start_addr;
    ctx.vsc_size = coh->cs_kernel_view_end_addr - coh->cs_kernel_view_start_addr;
#endif
    ctx.vsc_old_domain = current_desc->domain;
    ctx.vsc_new_domain = new_domain;
    // vsc_transition_generation will be set after lock
    // vsc_vclock might be from coherence state or proc context

    /* Check attributes based on the 'from' state */
    if ((current_desc->attributes & trans_rule->st_forbidden_attrs) != 0) {
        return EBUSY;  /* Forbidden attributes present */
    }
    if ((current_desc->attributes & trans_rule->st_required_attrs) !=
        trans_rule->st_required_attrs) {
        return EPERM;  /* Required attributes missing */
    }

    /* Run custom validator if present */
    if (trans_rule->st_validator != NULL) {
        error = trans_rule->st_validator(&ctx);
        if (error) {
            return error;
        }
    }

    /* Lock coherence for the critical section of transition */
    fsm_rw_wlock(&coh->cs_lock);

    /*
     * Re-check state: another thread/CPU might have changed it
     * between our initial check and acquiring the write lock.
     */
    if (current_desc->domain != ctx.vsc_old_domain) {
        fsm_rw_wunlock(&coh->cs_lock);
        return EAGAIN;  /* State changed, retry */
    }
    // Could also re-check attributes if they can change dynamically under cs_lock protection

    /* Perform the transition */
    current_desc->domain = new_domain;
    current_desc->transition_count++; // Assuming transition_count is part of semantic_descriptor
#ifdef VM_SEMANTIC_FSM_STANDALONE
    coh->cs_write_generation++; // Assuming cs_write_generation is part of coherence_state stub
    ctx.vsc_transition_generation = coh->cs_write_generation;
#else
    // In kernel, use the actual field from vm_coherence.h
    coh->cs_write_generation++; // This should be an atomic operation or protected
    ctx.vsc_transition_generation = coh->cs_write_generation;
#endif


    /* Execute transition action */
    if (trans_rule->st_action != NULL) {
        trans_rule->st_action(&ctx);
    }

    /* Notify all virtualization levels */
    // For standalone FSM test, use the stubs defined within this file's standalone block
    S_notify_transition_all_levels(coh, &ctx);

    /* If distributed, propagate transition */
#ifdef VM_SEMANTIC_FSM_STANDALONE
    // Use the local stub for propagation in standalone FSM test
    S_propagate_transition_network(coh, &ctx);
#else // Kernel code
    if (coh->cs_distributed) {
        propagate_transition_network(coh, &ctx); // Actual kernel function
    }
#endif

    fsm_rw_wunlock(&coh->cs_lock);

    return 0;
}

#ifdef VM_SEMANTIC_FSM_STANDALONE
// Dummy main for standalone compilation test
// We need simplified vm_semantic.h and vm_coherence.h for this to compile.
// Let's assume they define at least:
// enum semantic_domain { SEM_NONE, SEM_DATA, SEM_MESSAGE, SEM_HEAP, SEM_TEXT, SEM_MATRIX, SEM_MAX };
// struct semantic_descriptor { enum semantic_domain domain; uint32_t attributes; uint64_t version; uint32_t transition_count;};
// struct coherence_state { vm_offset_t cs_phys_base_stub; size_t cs_size_stub; struct semantic_descriptor cs_semantic; uint64_t cs_write_generation; pthread_rwlock_t cs_lock; int cs_distributed; /* other fields */ };

int main() {
    printf("Starting Semantic Transition Tests (Integrated into FSM Standalone)...\n\n");

    // Call the comprehensive test functions
    test_jit_compilation_transition();
    test_ipc_send_transition();
    test_ipc_receive_transition();
    test_matrix_to_message_transition();
    test_invalid_transition_no_rule();
    test_invalid_transition_forbidden_attr();
    test_invalid_transition_required_attr_missing();

    printf("\nAll Semantic Transition Tests Completed.\n");
    return 0;
}
#endif // VM_SEMANTIC_FSM_STANDALONE
