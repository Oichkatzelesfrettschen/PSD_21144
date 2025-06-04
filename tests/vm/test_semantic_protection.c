#include <assert.h>
#include <stdio.h>

#define VM_SEMANTIC_STANDALONE 1
#include "../../sys/vm/include/vm_semantic.h"

int main(void)
{
    struct semantic_descriptor d;
    d.domain = SEM_TEXT;
    d.attributes = 0;
    d.version = 1;
    assert(semantic_check_protection(&d, VM_PROT_READ | VM_PROT_EXECUTE) == 0);
    assert(semantic_check_protection(&d, VM_PROT_WRITE) == EPERM);
    d.domain = SEM_STACK;
    assert(semantic_check_protection(&d, VM_PROT_READ | VM_PROT_WRITE) == 0);
    assert(semantic_check_protection(&d, VM_PROT_EXECUTE) == EPERM);
    printf("semantic_protection ok\n");
    return 0;
}
