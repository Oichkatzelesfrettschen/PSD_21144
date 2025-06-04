#include <assert.h>
#include <stdio.h>

#define ZEROCOPY_STANDALONE 1
#define COHERENCE_STANDALONE 1
#define VM_SEMANTIC_STANDALONE 1
#include "../../sys/kern/kern_ipc_zerocopy.c"
#include "../../sys/vm/include/vm_semantic.h"

int main(void)
{
        struct vmspace vm1 = {0}, vm2 = {0};
        struct proc p1 = { &vm1 }, p2 = { &vm2 };
        struct zero_copy_channel *chan = NULL;
        int ret = create_zero_copy_channel(&p1, &p2, 4096, SEM_MESSAGE, &chan);
        assert(ret == 0 && chan != NULL);

        char buf[8] = "hi";
        ret = zero_copy_send(chan, buf, 3, 0);
        assert(ret == 0);
        assert(chan->zc_coh->write_generation >= 1);
        assert(chan->zc_coh->writer_level == 0);

        printf("zero_copy_ipc ok\n");
        return 0;
}
