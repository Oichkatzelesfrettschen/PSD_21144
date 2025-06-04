#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* Minimal stand-in types */
typedef unsigned long vm_offset_t;
typedef unsigned long vm_size_t;
typedef char*		  caddr_t;

#include "../../sys/vm/mmap_compat.h"

/* Stub vm_mmap implementation */
int vm_mmap(struct vm_map* map, unsigned long* addr, size_t len,
			int prot, int maxprot, int flags, char* handle, unsigned long pos) {
	(void) map;
	(void) len;
	(void) prot;
	(void) maxprot;
	(void) flags;
	(void) handle;
	(void) pos;
	*addr += 0x1000; /* pretend mapping succeeded */
	return 0;
}

int main(void) {
	struct vmspace vm	= { 0 };
	struct proc	   p	= { &vm };
	struct file	   f	= { 0 };
	void*		   addr = (void*) 0x0;
	int			   ret	= mmap_compat(&p, &addr, 4096, 0, 0, (int) (uintptr_t) &f, 0);
	assert(ret == 0);
	assert(addr != NULL);
	printf("mmap_compat ok\n");
	return 0;
}
