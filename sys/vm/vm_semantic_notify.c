/*
 * Semantic Transition Notification System
 */

#ifdef VM_SEMANTIC_NOTIFY_STANDALONE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Standalone stubs
typedef unsigned long vm_offset_t;
typedef unsigned long size_t; // Required by vm_semantic_context if it uses it
typedef int vm_prot_t;

struct proc { int p_pid; }; // Required by vm_semantic_context
// struct vmspace { int dummy; }; // Not directly used by notify's context
// struct vm_map { int dummy; }; // Not directly used by notify's context

#define EINVAL 22

// Stubs for semantic and coherence structures/functions
#include "include/vm_semantic.h"
#include "include/vm_coherence.h"
// #include "include/vm_coherence_dist.h" // Included via vm_coherence.h

// For vm_semantic_context definition
// This struct is central and defined in vm_semantic_fsm.c
// For standalone test of notify, we need its definition.
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
static void send_level_notification(int level, struct vm_semantic_context *ctx) {
    printf("STUB: Notifying level %d of transition from %d to %d for region [0x%lx-0x%lx]\n",
           level, ctx->vsc_old_domain, ctx->vsc_new_domain,
           (unsigned long)ctx->vsc_addr, (unsigned long)(ctx->vsc_addr + ctx->vsc_size));
}

static void update_level_protection(struct level_mapping *lm, enum semantic_domain new_domain) {
    vm_prot_t new_prot = 0;
    // Determine new protection based on domain
    switch (new_domain) {
        case SEM_TEXT:
            new_prot = VM_PROT_READ | VM_PROT_EXECUTE;
            break;
        case SEM_DATA:
        case SEM_HEAP:
        case SEM_STACK: // Stacks are typically R/W, non-exec
            new_prot = VM_PROT_READ | VM_PROT_WRITE;
            break;
        case SEM_MESSAGE: // Usually R/O for receiver, R/W for sender (complex)
        case SEM_MATRIX:  // Typically R/W
            new_prot = VM_PROT_READ | VM_PROT_WRITE; // Simplified
            break;
        default:
            new_prot = VM_PROT_READ; // Default to read-only
    }
    printf("STUB: Updating protection for level %d (vaddr 0x%lx) to new domain %d (prot 0x%x)\n",
           lm->lm_level, (unsigned long)lm->lm_vaddr, new_domain, new_prot);
    lm->lm_prot = new_prot; // Update the stubbed protection
    // In kernel: pmap_protect(pmap_for_level(lm->lm_level), lm->lm_vaddr, lm->lm_vaddr + size, new_prot);
}

// For propagate_transition_network
#define SEMANTIC_TRANS_MAGIC 0x53544D47 // "STMG"
struct semantic_transition_message {
    uint32_t stm_magic;
    enum semantic_domain stm_old_domain;
    enum semantic_domain stm_new_domain;
    uint64_t stm_generation;
    struct vector_clock stm_vclock; // Assuming direct struct copy for simplicity
};

// struct remote_node is now defined in vm_coherence.h (via _SYS_VM_COHERENCE_DIST_H_ guard)
// for standalone builds.

static void send_transition_message(struct remote_node *node, struct semantic_transition_message *msg) {
    printf("STUB: Sending transition message (gen %llu, %d -> %d) to remote node %d\n",
           (unsigned long long)msg->stm_generation, msg->stm_old_domain, msg->stm_new_domain, node->rn_node_id);
}

#else
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>   // For struct proc
#include <sys/lock.h>   // For rwlock_t if needed directly
#include <sys/malloc.h> // For M_TEMP if allocating messages

#include <vm/include/vm.h>
#include <vm/include/vm_semantic.h>
#include <vm/include/vm_coherence.h>
#include <vm/include/vm_coherence_dist.h> // For dist_coherence_state and vector_clock

// Forward declare vm_semantic_context (defined in vm_semantic_fsm.c)
struct vm_semantic_context;

// Prototypes for actual kernel functions (to be implemented elsewhere or stubbed)
static void send_level_notification(int level, struct vm_semantic_context *ctx);
static void update_level_protection(struct level_mapping *lm, enum semantic_domain new_domain);

#define SEMANTIC_TRANS_MAGIC 0x53544D47 // "STMG"
struct semantic_transition_message {
    uint32_t stm_magic;
    enum semantic_domain stm_old_domain;
    enum semantic_domain stm_new_domain;
    uint64_t stm_generation;
    struct vector_clock stm_vclock;
};

struct remote_node { /* ... as in standalone ... */ int rn_node_id; struct remote_node *rn_next; }; // Simplified
static void send_transition_message(struct remote_node *node, struct semantic_transition_message *msg);

#endif


/*
 * Notify all virtualization levels of a semantic transition.
 * This function is called after a semantic transition has been committed.
 */
void
notify_transition_all_levels(struct coherence_state *coh,
                           struct vm_semantic_context *ctx)
{
    struct level_mapping *lm;
    int level;

    printf("Notify: Iterating levels for transition from %d to %d.\n",
        ctx->vsc_old_domain, ctx->vsc_new_domain);

    for (level = 0; level < MAX_VIRT_LEVELS; level++) {
        lm = &coh->cs_mappings[level];

        if (!lm->lm_active) {
            continue;
        }

        /* Send a generic notification to this level's management entity */
        send_level_notification(level, ctx);

        /* Update protection based on the new domain */
        // This assumes the transition implies a protection change.
        // More sophisticated logic might be in the FSM action itself.
        update_level_protection(lm, ctx->vsc_new_domain);
    }
}

/*
 * Propagate semantic transition information to relevant network nodes.
 * This is called if the coherence state indicates the region is distributed.
 */
void
propagate_transition_network(struct coherence_state *coh,
                           struct vm_semantic_context *ctx)
{
#ifdef VM_SEMANTIC_NOTIFY_STANDALONE
    // In standalone, coh->cs_distributed is a simple int.
    // dist_coherence_state is part of coherence_state for standalone.
    struct dist_coherence_state *dist = (struct dist_coherence_state *)coh; // Risky cast, ensure layout matches
    if (!coh->cs_distributed) {
         printf("Notify: Region not distributed, skipping network propagation.\n");
        return;
    }
#else
    // In kernel, cs_distributed is a flag, and you'd cast to dist_coherence_state
    // if (coh->cs_semantic_state_ptr == NULL) return; // Should not happen if coh is valid
    if (!coh->cs_distributed) {
        return;
    }
    struct dist_coherence_state *dist = (struct dist_coherence_state *)coh;
#endif

    struct semantic_transition_message msg;
    struct remote_node *node_iter;

    printf("Notify: Propagating transition from %d to %d to network nodes.\n",
        ctx->vsc_old_domain, ctx->vsc_new_domain);

    /* Build transition message */
    msg.stm_magic = SEMANTIC_TRANS_MAGIC;
    msg.stm_old_domain = ctx->vsc_old_domain;
    msg.stm_new_domain = ctx->vsc_new_domain;
    msg.stm_generation = ctx->vsc_transition_generation;

    if (ctx->vsc_vclock) {
        // vector_clock_copy(&msg.stm_vclock, ctx->vsc_vclock); // If vector_clock_copy is available
        memcpy(&msg.stm_vclock, ctx->vsc_vclock, sizeof(struct vector_clock)); // Simple copy
    } else {
        memset(&msg.stm_vclock, 0, sizeof(struct vector_clock)); // Zero out if no vclock
    }

    /* Send to all nodes sharing this memory */
    // This assumes dcs_sharers is part of dist_coherence_state and is a TAILQ or similar
    // For standalone stub, we'll iterate a simple linked list if defined in the stub
#ifdef VM_SEMANTIC_NOTIFY_STANDALONE
    node_iter = dist->dcs_sharers_stub; // Assuming a stub field like 'dcs_sharers_stub'
    while(node_iter != NULL) {
        send_transition_message(node_iter, &msg);
        node_iter = node_iter->rn_next;
    }
#else
    // TAILQ_FOREACH(node_iter, &dist->dcs_sharers, rn_link) { // Kernel list iteration
    //     send_transition_message(node_iter, &msg);
    // }
    printf("STUB: Kernel network propagation loop for remote nodes would be here.\n");
#endif
}


#ifdef VM_SEMANTIC_NOTIFY_STANDALONE
// Dummy main for standalone compilation test
int main() {
    struct coherence_state dummy_coh_state;
    struct vm_semantic_context dummy_ctx;
    struct remote_node node1, node2;

    // Setup dummy_coh_state
    memset(&dummy_coh_state, 0, sizeof(dummy_coh_state));
    dummy_coh_state.cs_mappings[0].lm_level = 0;
    dummy_coh_state.cs_mappings[0].lm_active = 1; // True
    dummy_coh_state.cs_mappings[0].lm_vaddr = 0x10000;
    dummy_coh_state.cs_mappings[1].lm_level = 1;
    dummy_coh_state.cs_mappings[1].lm_active = 0; // False
    dummy_coh_state.cs_mappings[2].lm_level = 2;
    dummy_coh_state.cs_mappings[2].lm_active = 1; // True
    dummy_coh_state.cs_mappings[2].lm_vaddr = 0x30000;

    // Setup dummy_ctx
    memset(&dummy_ctx, 0, sizeof(dummy_ctx));
    dummy_ctx.vsc_coherence = &dummy_coh_state;
    dummy_ctx.vsc_old_domain = SEM_DATA;
    dummy_ctx.vsc_new_domain = SEM_TEXT;
    dummy_ctx.vsc_addr = 0x1000;
    dummy_ctx.vsc_size = 0x1000;
    dummy_ctx.vsc_transition_generation = 123;
    struct vector_clock dummy_vc = { .dummy = 1 };
    dummy_ctx.vsc_vclock = &dummy_vc;

    printf("Testing notify_transition_all_levels...\n");
    notify_transition_all_levels(&dummy_coh_state, &dummy_ctx);
    printf("notify_transition_all_levels finished.\n\n");

    printf("Testing propagate_transition_network (non-distributed)...\n");
    dummy_coh_state.cs_distributed = 0; // False
    propagate_transition_network(&dummy_coh_state, &dummy_ctx);
    printf("propagate_transition_network (non-distributed) finished.\n\n");

    printf("Testing propagate_transition_network (distributed)...\n");
    dummy_coh_state.cs_distributed = 1; // True
    // Setup a simple list for dcs_sharers_stub
    // Note: This part relies on how dist_coherence_state is stubbed in vm_coherence.h
    // For this test, we'll assume dist_coherence_state is directly part of coherence_state in standalone
    // and has a 'dcs_sharers_stub' field.
    struct dist_coherence_state* dist_state_ptr = (struct dist_coherence_state*)&dummy_coh_state;
    node1.rn_node_id = 101; node1.rn_next = &node2;
    node2.rn_node_id = 102; node2.rn_next = NULL;
    dist_state_ptr->dcs_sharers_stub = &node1; // Assuming this field in stub

    propagate_transition_network(&dummy_coh_state, &dummy_ctx);
    printf("propagate_transition_network (distributed) finished.\n");

    return 0;
}
#endif // VM_SEMANTIC_NOTIFY_STANDALONE
