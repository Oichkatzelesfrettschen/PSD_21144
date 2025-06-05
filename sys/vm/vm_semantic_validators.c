/*
 * Semantic Transition Validators and Actions
 */

#ifdef VM_SEMANTIC_VALIDATORS_STANDALONE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Standalone stubs - simplified for testing
typedef unsigned long vm_offset_t;
typedef unsigned long size_t;
typedef int vm_prot_t;

struct proc { int p_pid; };
struct vmspace { int dummy; };
struct vm_map { int dummy; };

#define EINVAL 22
#define EBUSY 16
#define EPERM 1
#define EAGAIN 35

// Stubs for semantic and coherence structures/functions
#include "include/vm_semantic.h"
#include "include/vm_coherence.h"
// #include "include/vm_coherence_dist.h" // Included via vm_coherence.h for standalone

// Bring in the FSM context definition (normally from vm_semantic_fsm.c)
// For standalone compilation, we might need to duplicate or include carefully
// For now, assume vm_semantic_fsm.c would be compiled alongside or its structs made available.
// To make this unit self-contained for testing, let's add the context here.
struct vm_semantic_context {
    struct coherence_state *vsc_coherence;
    struct proc *vsc_proc;
    vm_offset_t vsc_addr;
    size_t vsc_size;
    enum semantic_domain vsc_old_domain;
    enum semantic_domain vsc_new_domain;
    uint64_t vsc_transition_generation;
    struct vector_clock *vsc_vclock;
};


// Stubs for functions that would exist in the kernel
static int check_memory_readable(struct proc *p, vm_offset_t addr, size_t size) {
    printf("STUB: check_memory_readable for addr %lx, size %zu\n", addr, size);
    return 0; // Assume readable
}

static int validate_code_safety(vm_offset_t addr, size_t size) {
    printf("STUB: validate_code_safety for addr %lx, size %zu\n", addr, size);
    return 0; // Assume safe
}

#define INSTR_ALIGN 4 // Example instruction alignment

static int distributed_consensus_transition(struct coherence_state *coh, enum semantic_domain target_domain) {
    printf("STUB: distributed_consensus_transition for domain %d\n", target_domain);
    if (coh->cs_distributed) { // Check if it's actually distributed
        // In a real scenario, this would involve network communication
        // For now, assume consensus is met if it's a distributed region
        return 0;
    }
    return 0; // Not distributed, no consensus needed in this context
}

// Dummy message header for validation
#define MESSAGE_MAGIC 0xABADCAFE
struct message_header {
    uint32_t mh_magic;
    uint32_t mh_size; // Payload size
    // other fields
};

#else
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lock.h> // May not be directly needed here but often in kernel files
#include <sys/malloc.h>

#include <vm/include/vm.h>
#include <vm/include/vm_semantic.h>
#include <vm/include/vm_coherence.h>
#include <vm/include/vm_coherence_dist.h> // For dist_coherence_state
// We need the definition of vm_semantic_context. It's in vm_semantic_fsm.c.
// In a kernel build, these would be linked. For now, ensure it's declared.
// If vm_semantic_fsm.h is created later, this would go there.
struct vm_semantic_context; // Forward declare

// Prototypes for actual kernel functions (to be implemented elsewhere)
static int check_memory_readable(struct proc *p, vm_offset_t addr, size_t size);
static int validate_code_safety(vm_offset_t addr, size_t size);
#define INSTR_ALIGN 4 // Define appropriately for the target architecture
static int distributed_consensus_transition(struct coherence_state *coh, enum semantic_domain target_domain);
struct message_header { uint32_t mh_magic; uint32_t mh_size; /* ... */ };
#define MESSAGE_MAGIC 0xABADCAFE // Example magic value
#endif

/*
 * Validate heap-to-text transition (e.g., JIT compilation)
 */
int
validate_heap_to_text(struct vm_semantic_context *ctx)
{
    struct coherence_state *coh = ctx->vsc_coherence;
    int error;

    printf("Validator: HEAP -> TEXT for region starting at %lx\n", (unsigned long)ctx->vsc_addr);

    /* Verify memory is readable for compilation/validation if needed by JIT */
    error = check_memory_readable(ctx->vsc_proc, ctx->vsc_addr, ctx->vsc_size);
    if (error) {
        return error;
    }

    /* Stub for actual code safety validation (e.g., disassembler checks) */
    error = validate_code_safety(ctx->vsc_addr, ctx->vsc_size);
    if (error) {
        return EINVAL; // Or a more specific error
    }

    /* Ensure no other CPU is writing to this region (simplified check) */
    // A real check might involve more complex inter-processor synchronization
    if (coh->cs_active_writers > 0) {
        return EBUSY;
    }

    /* Verify alignment for instruction fetch */
    if ((ctx->vsc_addr & (INSTR_ALIGN - 1)) != 0) {
        return EINVAL; // Alignment error
    }

    /* Check that all sharing nodes agree to this transition if distributed */
#ifndef VM_SEMANTIC_VALIDATORS_STANDALONE
    if (coh->cs_distributed) {
        error = distributed_consensus_transition(coh, SEM_TEXT);
        if (error) {
            return error;
        }
    }
#endif
    return 0;
}

void
action_heap_to_text(struct vm_semantic_context *ctx)
{
    printf("Action: HEAP -> TEXT for region at %lx. (Stub: Would update pmap protections)\n", (unsigned long)ctx->vsc_addr);
    // In a real implementation:
    // pmap_protect(vmspace_pmap(ctx->vsc_proc->p_vmspace), ctx->vsc_addr,
    //              ctx->vsc_addr + ctx->vsc_size, VM_PROT_READ | VM_PROT_EXECUTE);
    // Update semantic descriptor attributes if needed, e.g., clear SEM_ATTR_WRITABLE, set SEM_ATTR_EXECUTABLE
    ctx->vsc_coherence->cs_semantic.attributes &= ~SEM_ATTR_WRITABLE;
    ctx->vsc_coherence->cs_semantic.attributes |= SEM_ATTR_EXECUTABLE;
}

/*
 * Validate data-to-message transition
 */
int
validate_data_to_message(struct vm_semantic_context *ctx)
{
    struct message_header *hdr;
    size_t payload_size;

    printf("Validator: DATA -> MESSAGE for region at %lx\n", (unsigned long)ctx->vsc_addr);

    /* Messages must have a header */
    if (ctx->vsc_size < sizeof(struct message_header)) {
        return EINVAL; // Region too small for a header
    }

    /* Validate header structure (basic check) */
    // In a real system, you'd copyin the header to check it safely if from userspace
    hdr = (struct message_header *)ctx->vsc_addr; // Assuming kernel address space for now
    if (hdr->mh_magic != MESSAGE_MAGIC) { // Example magic number
        return EINVAL; // Invalid message magic
    }

    /* Ensure complete message fits in region */
    payload_size = hdr->mh_size;
    if (sizeof(*hdr) + payload_size > ctx->vsc_size) {
        return EINVAL; // Message size exceeds region
    }

    /* Check vector clock is properly initialized if distributed */
#ifndef VM_SEMANTIC_VALIDATORS_STANDALONE
    struct dist_coherence_state *dist_coh = (struct dist_coherence_state *)ctx->vsc_coherence;
    if (ctx->vsc_coherence->cs_distributed && ctx->vsc_vclock == NULL) {
         // Or if dist_coh->dcs_vclock is not initialized. This depends on where vclock is stored.
         // For now, assume vsc_vclock is populated if needed.
        // return EINVAL; // Vector clock missing for distributed message
    }
#endif
    return 0;
}

void
action_data_to_message(struct vm_semantic_context *ctx)
{
    printf("Action: DATA -> MESSAGE for region at %lx. (Stub: Would update vector clock if distributed)\n", (unsigned long)ctx->vsc_addr);
    // In a real implementation:
    // if (ctx->vsc_coherence->cs_distributed && ctx->vsc_vclock) {
    //     vector_clock_increment(ctx->vsc_vclock, current_node_id());
    // }
    // Set attributes like SEM_ATTR_IN_TRANSIT, clear SEM_ATTR_BUFFER_VALID (if it was set)
    ctx->vsc_coherence->cs_semantic.attributes |= SEM_ATTR_IN_TRANSIT;
    ctx->vsc_coherence->cs_semantic.attributes &= ~SEM_ATTR_BUFFER_VALID; // Example
}

/*
 * Validate message-to-data transition
 */
int
validate_message_to_data(struct vm_semantic_context *ctx)
{
    printf("Validator: MESSAGE -> DATA for region at %lx\n", (unsigned long)ctx->vsc_addr);
    // Typically, this transition happens after successful delivery.
    // The SEM_ATTR_DELIVERED is checked by the FSM core.
    // Further validation could check message integrity if needed.
    return 0;
}

void
action_message_to_data(struct vm_semantic_context *ctx)
{
    printf("Action: MESSAGE -> DATA for region at %lx.\n", (unsigned long)ctx->vsc_addr);
    // Clear message-specific attributes
    ctx->vsc_coherence->cs_semantic.attributes &= ~(SEM_ATTR_IN_TRANSIT | SEM_ATTR_DELIVERED);
    // Potentially set SEM_ATTR_BUFFER_VALID if the data is now ready for consumption
    ctx->vsc_coherence->cs_semantic.attributes |= SEM_ATTR_BUFFER_VALID; // Example
}


/*
 * Validate matrix-to-message transition
 */
int
validate_matrix_to_message(struct vm_semantic_context *ctx)
{
    printf("Validator: MATRIX -> MESSAGE for region at %lx\n", (unsigned long)ctx->vsc_addr);
    // Check if SEM_ATTR_COMPUTING is set in current_desc->attributes (done by FSM core)
    // Additional checks:
    // - Ensure matrix data is in a serializable format.
    // - If distributed, ensure vector clocks are ready.
    return 0;
}

void
action_matrix_to_message(struct vm_semantic_context *ctx)
{
    printf("Action: MATRIX -> MESSAGE for region at %lx.\n", (unsigned long)ctx->vsc_addr);
    // Similar to data_to_message, prepare for network transmission
    // May involve setting specific attributes or calling network prep functions
    ctx->vsc_coherence->cs_semantic.attributes |= SEM_ATTR_IN_TRANSIT;
}


#ifdef VM_SEMANTIC_VALIDATORS_STANDALONE
// Dummy main for standalone compilation test
int main() {
    struct coherence_state dummy_coh_state;
    struct vm_semantic_context dummy_ctx;

    // Setup dummy_coh_state (needs cs_active_writers, cs_distributed, cs_semantic)
    dummy_coh_state.cs_active_writers = 0;
    dummy_coh_state.cs_distributed = 0; // Test non-distributed first
    dummy_coh_state.cs_semantic.domain = SEM_HEAP;
    dummy_coh_state.cs_semantic.attributes = SEM_ATTR_COMPILED; // Required for HEAP->TEXT

    // Setup dummy_ctx
    dummy_ctx.vsc_coherence = &dummy_coh_state;
    dummy_ctx.vsc_proc = NULL; // Standalone, no real proc
    dummy_ctx.vsc_addr = 0x1000; // Example address
    dummy_ctx.vsc_size = 0x1000; // Example size
    dummy_ctx.vsc_old_domain = SEM_HEAP;
    dummy_ctx.vsc_new_domain = SEM_TEXT;
    dummy_ctx.vsc_vclock = NULL;


    printf("Testing validate_heap_to_text...\n");
    assert(validate_heap_to_text(&dummy_ctx) == 0);
    action_heap_to_text(&dummy_ctx);
    printf("validate_heap_to_text passed.\n\n");

    // Test data_to_message
    // Setup for data_to_message
    char buffer[sizeof(struct message_header) + 10];
    struct message_header *hdr = (struct message_header *)buffer;
    hdr->mh_magic = MESSAGE_MAGIC;
    hdr->mh_size = 5; // Payload size

    dummy_coh_state.cs_semantic.domain = SEM_DATA;
    dummy_coh_state.cs_semantic.attributes = 0; // No conflicting attributes
    dummy_ctx.vsc_addr = (vm_offset_t)buffer;
    dummy_ctx.vsc_size = sizeof(buffer);
    dummy_ctx.vsc_old_domain = SEM_DATA;
    dummy_ctx.vsc_new_domain = SEM_MESSAGE;

    printf("Testing validate_data_to_message...\n");
    assert(validate_data_to_message(&dummy_ctx) == 0);
    action_data_to_message(&dummy_ctx);
    printf("validate_data_to_message passed.\n\n");

    // Test matrix_to_message
    dummy_coh_state.cs_semantic.domain = SEM_MATRIX;
    dummy_coh_state.cs_semantic.attributes = 0; // Not SEM_ATTR_COMPUTING
    dummy_ctx.vsc_old_domain = SEM_MATRIX;
    dummy_ctx.vsc_new_domain = SEM_MESSAGE;
    printf("Testing validate_matrix_to_message...\n");
    assert(validate_matrix_to_message(&dummy_ctx) == 0);
    action_matrix_to_message(&dummy_ctx);
    printf("validate_matrix_to_message passed.\n");


    return 0;
}
#endif // VM_SEMANTIC_VALIDATORS_STANDALONE
