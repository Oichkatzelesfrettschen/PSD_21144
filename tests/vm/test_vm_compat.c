#include <assert.h>
#include <stddef.h>
#include <stdio.h>

/* Minimal stand-in structures for testing */
struct vm_data {
	size_t psx_dsize;
	void*  psx_daddr;
};

struct vm_stack {
	size_t psx_ssize;
	void*  psx_saddr;
};

struct vm_text {
	size_t psx_tsize;
	void*  psx_taddr;
};

struct vm_pseudo_segment {
	struct vmspace*	 ps_vmspace;
	struct vm_data*	 ps_data;
	struct vm_stack* ps_stack;
	struct vm_text*	 ps_text;
	void*			 ps_minsaddr;
	void*			 ps_maxsaddr;
#define ps_dsize ps_data->psx_dsize
#define ps_ssize ps_stack->psx_ssize
#define ps_tsize ps_text->psx_tsize
#define ps_daddr ps_data->psx_daddr
#define ps_saddr ps_stack->psx_saddr
#define ps_taddr ps_text->psx_taddr
};

struct vmspace {
	struct vm_pseudo_segment vm_psegment;
};

struct proc {
	struct vmspace* p_vmspace;
};

#define VM_COMPAT_STANDALONE
#include "../../sys/vm/vm_compat.h"

int main(void) {
	struct vm_data	data  = { 0 };
	struct vm_stack stack = { 0 };
	struct vm_text	text  = { 0 };
	struct vmspace	vm	  = { 0 };
	struct proc		p	  = { 0 };

	vm.vm_psegment.ps_data	= &data;
	vm.vm_psegment.ps_stack = &stack;
	vm.vm_psegment.ps_text	= &text;
	p.p_vmspace				= &vm;

	assert(PROC_TO_VM(&p) == &vm);
	assert(PROC_DATA_SIZE(&p) == 0);
	printf("vm_compat ok\n");
	return 0;
}
