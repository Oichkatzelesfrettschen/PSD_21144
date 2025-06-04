/* Lightweight semantic registry for memory regions. */

#ifdef VM_SEMANTIC_STANDALONE
#include <stdlib.h>
#include <string.h>
typedef unsigned long vm_offset_t;
typedef struct rwlock { int dummy; } rwlock_t;
static inline void rw_init(rwlock_t *l, const char *n) { (void)l; (void)n; }
static inline void rw_wlock(rwlock_t *l) { (void)l; }
static inline void rw_wunlock(rwlock_t *l) { (void)l; }
static inline void rw_rlock(rwlock_t *l) { (void)l; }
static inline void rw_runlock(rwlock_t *l) { (void)l; }
#define malloc(s, t, f) calloc(1, s)
#define free(p, t) free(p)
#define M_ZERO 0
#define EEXIST 17
#define EPERM 1
#define EINVAL 22
#define VM_PROT_READ  0x1
#define VM_PROT_WRITE 0x2
#define VM_PROT_EXECUTE 0x4
#else
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/lock.h>
#include <vm/vm.h>
#endif
#ifdef VM_SEMANTIC_STANDALONE
#include "vm_semantic.h"
#else
#include <vm/vm_semantic.h>
#endif

/* Linked list head for semantic regions. */
static struct semantic_entry *semantic_head;
static rwlock_t               semantic_lock;

/* Initialize the registry. */
void
vm_semantic_init(void)
{
        semantic_head = NULL;
        rw_init(&semantic_lock, "semreg");
}

/* Register a region with semantic information. */
int
vm_semantic_register(vm_offset_t start, vm_offset_t end,
    enum semantic_domain dom, uint32_t attr)
{
        struct semantic_entry *e;

        rw_wlock(&semantic_lock);
        for (e = semantic_head; e != NULL; e = e->next) {
                if (!(end <= e->start || start >= e->end)) {
                        rw_wunlock(&semantic_lock);
                        return EEXIST;
                }
        }
        e = malloc(sizeof(*e), M_VMSEM, M_WAITOK | M_ZERO);
        e->start = start;
        e->end = end;
        e->desc.domain = dom;
        e->desc.attributes = attr;
        e->desc.version = 1;
        e->next = semantic_head;
        semantic_head = e;
        rw_wunlock(&semantic_lock);
        return 0;
}

/* Lookup a region covering a range. */
struct semantic_entry *
vm_semantic_lookup(vm_offset_t start, vm_offset_t end)
{
        struct semantic_entry *e;

        rw_rlock(&semantic_lock);
        for (e = semantic_head; e != NULL; e = e->next) {
                if (start >= e->start && end <= e->end) {
                        rw_runlock(&semantic_lock);
                        return e;
                }
        }
        rw_runlock(&semantic_lock);
        return NULL;
}

/* Check that protection flags are compatible with the semantic domain. */
int
semantic_check_protection(struct semantic_descriptor *desc, vm_prot_t prot)
{
        switch (desc->domain) {
        case SEM_TEXT:
                if (prot & VM_PROT_WRITE)
                        return EPERM;
                if (!(prot & VM_PROT_EXECUTE))
                        return EINVAL;
                break;
        case SEM_STACK:
                if (prot & VM_PROT_EXECUTE)
                        return EPERM;
                if (!(prot & (VM_PROT_READ | VM_PROT_WRITE)))
                        return EINVAL;
                break;
        case SEM_MESSAGE:
                if (!(prot & VM_PROT_READ))
                        return EINVAL;
                break;
        case SEM_MATRIX:
                if (prot & VM_PROT_EXECUTE)
                        return EPERM;
                break;
        case SEM_DATA:
        case SEM_MAX:
                break;
        }
        return 0;
}
