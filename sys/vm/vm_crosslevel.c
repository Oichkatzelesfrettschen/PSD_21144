/*
 * Cross-level memory sharing helpers.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/lock.h>
#include <vm/vm.h>
#include <vm/vm_semantic.h>
#include <vm/vm_crosslevel.h>

/*
 * Establish sharing of a memory range between different virtualization
 * levels. The caller specifies the target level number. For simplicity
 * this stub only records the physical address and semantic descriptor.
 */
int
vm_crosslevel_establish(struct proc *p, vm_offset_t addr, size_t len,
    int target_level, struct crosslevel_share **share_out)
{
        struct crosslevel_share *sh;
        struct semantic_entry *sem;
        vm_paddr_t phys;
        int error;

        (void)target_level;

        /* look up semantic information */
        sem = vm_semantic_lookup(addr, addr + len);
        if (sem == NULL)
                return EINVAL;

        /* translate virtual address to physical */
        error = vm_extract_physical(p->p_vmspace, addr, len, &phys);
        if (error)
                return error;

        sh = malloc(sizeof(*sh), M_VMSHARE, M_WAITOK | M_ZERO);
        sh->cs_semantic = &sem->desc;
        sh->cs_phys_base = phys;
        sh->cs_refcount = 1;
        rw_init(&sh->cs_lock, "crosslevel");

        *share_out = sh;
        return 0;
}

/*
 * Map the shared region into the specified process. For this minimal
 * implementation the returned address is simply the physical address
 * cast back to a pointer.
 */
int
vm_crosslevel_map_local(struct crosslevel_share *share, struct proc *p,
    vm_offset_t *addr_out)
{
        (void)p;
        *addr_out = share->cs_phys_base;
        return 0;
}
