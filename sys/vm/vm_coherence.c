/*
 * Lightweight coherence tracking for cross-level memory sharing.
 */

#ifdef COHERENCE_STANDALONE
#include <stdlib.h>
#include <string.h>
#include "include/vm_coherence.h"
static inline void rw_init(int *l, const char *n) { (void)l; (void)n; }
static inline void rw_wlock(int *l) { (void)l; }
static inline void rw_wunlock(int *l) { (void)l; }
#define ENOMEM 12
#define EINVAL 22
#define EEXIST 17
#else
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <vm/include/vm_extern.h>
#include <vm/include/vm.h>
#include <vm/include/vm_page.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_map.h>
#include <vm/include/vm_page.h>
#include <vm/include/vm_object.h>
#include <vm/include/vm_pager.h>
#include <vm/include/vm_psegment.h>
#include <vm/include/vm_segment.h>
#include <vm/include/vm_semantic.h>
#include <vm/include/vm_coherence.h>
#endif

struct coherence_state_list {
    struct coherence_state *head;
};

static struct coherence_state_list coh_list;
static int                         coh_lock;
static uint64_t                    coh_gen;

/* Initialize global coherence tracking. */
void
vm_coherence_init(void)
{
    coh_list.head = NULL;
    rw_init(&coh_lock, "coherence");
    coh_gen = 1;
}

/* Allocate and record a new coherence state. */
int
vm_coherence_establish(vm_paddr_t phys, size_t size,
    struct semantic_descriptor *sem, struct coherence_state **state_out)
{
    struct coherence_state *s;

    s = calloc(1, sizeof(*s));
    if (s == NULL)
        return ENOMEM;
    s->phys_base = phys;
    s->size = size;
    if (sem != NULL)
        memcpy(&s->semantic, sem, sizeof(*sem));
    s->write_generation = coh_gen++;
    s->writer_level = -1;

    rw_wlock(&coh_lock);
    s->mappings[0].level = 0;
    s->mappings[1].level = 1;
    s->mappings[2].level = 2;
    s->mappings[0].active = 0;
    s->mappings[1].active = 0;
    s->mappings[2].active = 0;
    s->next = coh_list.head;
    coh_list.head = s;
    rw_wunlock(&coh_lock);

    *state_out = s;
    return 0;
}

/* Map a coherent region at the requested level. */
int
vm_coherence_map(struct coherence_state *s, int level,
    vm_offset_t addr, vm_prot_t prot)
{
    struct level_mapping *lm;

    if (level < 0 || level >= MAX_VIRT_LEVELS)
        return EINVAL;
    lm = &s->mappings[level];
    if (lm->active)
        return EEXIST;

    lm->vaddr = addr;
    lm->prot = prot;
    lm->active = 1;
    lm->generation = s->write_generation;
    return 0;
}

/* Mark the region as written by a level. */
void
vm_coherence_mark_write(struct coherence_state *s, int level)
{
    s->write_generation++;
    s->writer_level = level;
}
