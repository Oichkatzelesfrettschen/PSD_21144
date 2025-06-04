#ifndef _VM_COHERENCE_H_
#define _VM_COHERENCE_H_

#include <stddef.h>
#include <stdint.h>
#ifdef COHERENCE_STANDALONE
#include <stdlib.h>
#include <string.h>
#endif

#include "vm_semantic.h"

#ifdef COHERENCE_STANDALONE
typedef unsigned long vm_paddr_t;
typedef unsigned long vm_offset_t;
#define ENOMEM 12
#define EINVAL 22
#define EEXIST 17
#endif

#define MAX_VIRT_LEVELS 3

struct level_mapping {
    int      level;       /* virtualization level */
    vm_offset_t vaddr;    /* virtual address at level */
    vm_prot_t prot;       /* protection flags */
    int      active;      /* mapping active? */
    uint64_t generation;  /* generation when mapped */
};

struct coherence_state {
    vm_paddr_t                phys_base;    /* physical base */
    size_t                    size;         /* region size */
    struct semantic_descriptor semantic;    /* semantic info */
    struct level_mapping      mappings[MAX_VIRT_LEVELS];
    uint64_t                  write_generation; /* last write */
    int                       writer_level;     /* level of writer */
    struct coherence_state    *next;            /* global list link */
};

#ifndef COHERENCE_STANDALONE
void vm_coherence_init(void);
int vm_coherence_establish(vm_paddr_t phys, size_t size,
    struct semantic_descriptor *sem,
    struct coherence_state **state_out);
int vm_coherence_map(struct coherence_state *state, int level,
    vm_offset_t addr, vm_prot_t prot);
void vm_coherence_mark_write(struct coherence_state *state, int level);
#endif
#ifdef COHERENCE_STANDALONE
static inline void vm_coherence_init(void) {}
static inline int vm_coherence_establish(vm_paddr_t p, size_t s,
    struct semantic_descriptor *se, struct coherence_state **out)
{
    (void)p;
    *out = calloc(1, sizeof(struct coherence_state));
    if (*out == NULL)
        return ENOMEM;
    (*out)->size = s;
    if (se)
        memcpy(&(*out)->semantic, se, sizeof(*se));
    return 0;
}
static inline int vm_coherence_map(struct coherence_state *st, int l,
    vm_offset_t a, vm_prot_t pr)
{
    if (st) {
        st->mappings[l].active = 1;
        st->mappings[l].vaddr = a;
        st->mappings[l].prot = pr;
    }
    return 0;
}
static inline void vm_coherence_mark_write(struct coherence_state *st, int l)
{
    if (st) {
        st->write_generation++;
        st->writer_level = l;
    }
}
#endif

#endif /* _VM_COHERENCE_H_ */
