#include <assert.h>
#include <stddef.h>
#include <stdio.h>

typedef unsigned long segsz_t;
typedef char *caddr_t;

/* Minimal stand-in structures for testing */
struct vm_data {
	size_t psx_dsize;
	void *psx_daddr;
};
struct vm_stack {
	size_t psx_ssize;
	void *psx_saddr;
};
struct vm_text {
	size_t psx_tsize;
	void *psx_taddr;
};
struct vm_pseudo_segment {
	struct vmspace *ps_vmspace;
	struct vm_data *ps_data;
	struct vm_stack *ps_stack;
	struct vm_text *ps_text;
	void *ps_minsaddr;
	void *ps_maxsaddr;
};
struct vmspace {
	struct vm_pseudo_segment vm_psegment;
};
struct proc {
	struct vmspace *p_vmspace;
	void *p_ovlspace;
};
static int lock_count;
static int mapout_calls;
static int mapin_calls;

/* Token stubs */
struct lwkt_token {
	int dummy;
};
static void lwkt_token_init(struct lwkt_token *t, const char *n) {
	(void)t;
	(void)n;
}
static void lwkt_gettoken(struct lwkt_token *t) {
	(void)t;
	lock_count++;
}
static void lwkt_reltoken(struct lwkt_token *t) {
	(void)t;
	lock_count--;
}

/* VM and overlay stubs */
static struct vmspace *vmspace_fork(struct vmspace *vm) { return vm; }
static int cpu_fork(struct proc *p1, struct proc *p2) {
	(void)p1;
	(void)p2;
	return 0;
}
static void ovlspace_mapout(void *ovl) {
	(void)ovl;
	mapout_calls++;
}
static void ovlspace_mapin(void *ovl) {
	(void)ovl;
	mapin_calls++;
}

#define VM_COMPAT_STANDALONE
#include "../../sys/vm/vm_compat.h"

/* Minimal copy of kern_fork_compat with explicit initialization. */
static struct lwkt_token kf_token;
static int kf_initialized;

static void kern_fork_compat_init(void) {
	if (!kf_initialized) {
		lwkt_token_init(&kf_token, "kf");
		kf_initialized = 1;
	}
}

static int kern_fork_compat(struct proc *parent, struct proc *child,
							int isvfork) {
	(void)isvfork;

	lwkt_gettoken(&kf_token);
	ovlspace_mapout(parent->p_ovlspace);
	child->p_vmspace = vmspace_fork(parent->p_vmspace);
	ovlspace_mapin(child->p_ovlspace);
	lwkt_reltoken(&kf_token);
	return cpu_fork(parent, child);
}

int main(void) {
	struct vmspace vm = {0};
	struct proc parent = {&vm, &vm};
	struct proc child = {NULL, &vm};

	kern_fork_compat_init();

	int ret = kern_fork_compat(&parent, &child, 0);

	assert(ret == 0);
	assert(child.p_vmspace == &vm);
	assert(mapout_calls == 1);
	assert(mapin_calls == 1);
	assert(lock_count == 0);

	printf("fork_compat ok\n");
	return 0;
}
