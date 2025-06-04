#include <assert.h>
#include <stdio.h>

#define VM_SEMANTIC_STANDALONE 1
#include "../../sys/vm/include/vm_semantic.h"

int main(void)
{
        vm_semantic_init();
        int ret = vm_semantic_register(0x1000, 0x2000, SEM_DATA, 0);
        assert(ret == 0);
        struct semantic_entry *se = vm_semantic_lookup(0x1000, 0x1800);
        assert(se != NULL && se->desc.domain == SEM_DATA);
        printf("semantic_memory ok\n");
        return 0;
}
