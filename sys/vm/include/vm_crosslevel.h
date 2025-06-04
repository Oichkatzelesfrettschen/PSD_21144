#ifndef _VM_CROSSLEVEL_H_
#define _VM_CROSSLEVEL_H_

#include <vm/vm_semantic.h>

#ifdef VM_CROSSLEVEL_STANDALONE
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
struct proc { struct vmspace *p_vmspace; };
struct vmspace { int dummy; };
static inline int vm_extract_physical(struct vmspace *vm, vm_offset_t a,
    size_t l, vm_paddr_t *pa) { (void)vm; (void)a; (void)l; *pa = 0; return 0; }
#define M_VMSHARE 0
#define M_WAITOK 0
#else
#include <sys/proc.h>
#include <sys/malloc.h>
#endif

struct crosslevel_share {
        struct semantic_descriptor *cs_semantic; /* semantic info */
        vm_paddr_t                 cs_phys_base; /* physical address */
        vm_offset_t                cs_l0_base;   /* kernel virtual */
        vm_offset_t                cs_l1_base;   /* vkernel virtual */
        vm_offset_t                cs_l2_base;   /* process virtual */
        uint32_t                   cs_refcount;  /* reference count */
        struct rwlock              cs_lock;      /* state lock */
};

int vm_crosslevel_establish(struct proc *p, vm_offset_t addr, size_t len,
            int target_level, struct crosslevel_share **share_out);
int vm_crosslevel_map_local(struct crosslevel_share *share, struct proc *p,
            vm_offset_t *addr_out);

#endif /* _VM_CROSSLEVEL_H_ */
