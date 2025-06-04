CC=clang
CFLAGS=-Wall -Wextra

all: test_vm_compat test_kern_fork_compat test_sched_preempt \
     test_mmap_compat test_semantic_signals test_zero_copy_ipc \
     test_semantic_memory test_semantic_protection \
     test_coherence_dist

test_vm_compat: test_vm_compat.c
	$(CC) $(CFLAGS) -DVM_COMPAT_STANDALONE -o $@ $<

test_kern_fork_compat: test_kern_fork_compat.c
	$(CC) $(CFLAGS) -o $@ $<

test_sched_preempt: test_sched_preempt.c ../../sys/kern/sched_preempt.c
	$(CC) $(CFLAGS) -DSCHED_PREEMPT_STANDALONE ../../sys/kern/sched_preempt.c $< -o $@

test_mmap_compat: test_mmap_compat.c
	$(CC) $(CFLAGS) -DMMAP_COMPAT_STANDALONE -o $@ $<

test_semantic_signals: test_semantic_signals.c
	$(CC) $(CFLAGS) -DSIG_UNIFIED_STANDALONE -o $@ $<

test_zero_copy_ipc: test_zero_copy_ipc.c
	$(CC) $(CFLAGS) -DZEROCOPY_STANDALONE -o $@ $<

test_semantic_memory: test_semantic_memory.c ../../sys/vm/vm_semantic.c
	$(CC) $(CFLAGS) -DVM_SEMANTIC_STANDALONE -I../../sys/vm/include \
../../sys/vm/vm_semantic.c $< -o $@

test_semantic_protection: test_semantic_protection.c ../../sys/vm/vm_semantic.c
	$(CC) $(CFLAGS) -DVM_SEMANTIC_STANDALONE -I../../sys/vm/include \
../../sys/vm/vm_semantic.c $< -o $@

test_coherence_dist: test_coherence_dist.c ../../sys/vm/vm_coherence_dist.c
	$(CC) $(CFLAGS) -DCOHERENCE_STANDALONE -DVM_SEMANTIC_STANDALONE \
	    -I../../sys/vm/include \
	    ../../sys/vm/vm_coherence_dist.c $< -o $@

check: all
	./test_vm_compat
	./test_kern_fork_compat
	./test_sched_preempt
	./test_mmap_compat
	./test_semantic_signals
	./test_zero_copy_ipc
	./test_semantic_memory
	./test_semantic_protection
	./test_coherence_dist

clean:
	rm -f test_vm_compat test_kern_fork_compat test_sched_preempt \
	test_mmap_compat test_semantic_signals test_zero_copy_ipc \
	test_semantic_memory test_semantic_protection \
	test_coherence_dist
