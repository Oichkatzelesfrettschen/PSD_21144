#ifndef _SYS_MMAP_COMPAT_H_
#define _SYS_MMAP_COMPAT_H_

#ifndef MMAP_COMPAT_STANDALONE
#include <sys/types.h>
#include <sys/proc.h>
#include <sys/mman.h>
#else
/* Minimal type definitions for unit tests */
typedef unsigned long size_t;
typedef long off_t;
struct vm_map { int dummy; };
struct vmspace { struct vm_map vm_map; };
struct proc { struct vmspace *p_vmspace; };
struct file { int dummy; };
int vm_mmap(struct vm_map *, unsigned long *, size_t, int, int,
    int, char *, unsigned long);
#endif

/*
 * mmap_compat - compatibility wrapper for vm_mmap().
 * Maintains 2.11BSD file descriptor semantics while using
 * the 4.4BSD VM system underneath.
 */
#ifdef MMAP_COMPAT_STANDALONE
static inline int
mmap_compat(struct proc *p, void **addr, size_t len, int prot,
    int flags, int fd, off_t pos)
{
    unsigned long a = (unsigned long)*addr;
    int error = vm_mmap(&p->p_vmspace->vm_map, &a, len, prot,
        0, flags, (char *)(unsigned long)fd, pos);
    *addr = (void *)a;
    return error;
}
#else
int mmap_compat(struct proc *p, void **addr, size_t len, int prot,
    int flags, int fd, off_t pos);
#endif

#endif /* _SYS_MMAP_COMPAT_H_ */
