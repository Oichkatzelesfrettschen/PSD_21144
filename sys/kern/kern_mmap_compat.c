/*
 * The 3-Clause BSD License
 * Copyright (c) 2024 The PSD_21144 Contributors
 * All rights reserved.
 */

#ifndef MMAP_COMPAT_STANDALONE
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/proc.h>
#else
/* Minimal stand-ins for unit tests */
typedef unsigned long size_t;
typedef long		  off_t;
struct vm_map {
	int dummy;
};
struct vmspace {
	struct vm_map vm_map;
};
struct proc {
	struct vmspace* p_vmspace;
};
struct file {
	int dummy;
};
#define VM_PROT_ALL 0
#endif
#include <vm/include/vm.h>
#include <vm/include/vm_extern.h>
#include <vm/mmap_compat.h>

/*
 * mmap_compat - map a file into memory using 4.4BSD's vm_mmap while
 * keeping the classic 2.11BSD file-descriptor interface.
 */
int mmap_compat(struct proc* p, void** addr, size_t len, int prot,
				int flags, int fd, off_t pos) {
	vm_offset_t a;

	/*
	 * Translate the file descriptor to a kernel file pointer.  This
	 * placeholder assumes 'fd' is already a valid pointer for the
	 * standalone tests.  A real implementation would call fdget().
	 */
	struct file* fp = (struct file*) (uintptr_t) fd;

	a = (vm_offset_t) *addr;

	/* Call the underlying VM to perform the mapping. */
	int error = vm_mmap(&p->p_vmspace->vm_map, &a, len, prot,
						VM_PROT_ALL, flags, (caddr_t) fp, pos);

	*addr = (void*) a;
	return error;
}
