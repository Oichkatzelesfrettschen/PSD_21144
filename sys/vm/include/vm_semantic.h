#ifndef _VM_SEMANTIC_H_
#define _VM_SEMANTIC_H_

#ifdef VM_SEMANTIC_STANDALONE
#include <stddef.h>
#include <stdint.h>
typedef unsigned long vm_offset_t;
typedef int vm_prot_t;
#define VM_PROT_READ  0x1
#define VM_PROT_WRITE 0x2
#define VM_PROT_EXECUTE 0x4
#define EPERM 1
#define EINVAL 22
#define M_VMSEM 0
#define M_WAITOK 0
#else
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/tree.h>
#endif

/* Semantic domains */
enum semantic_domain {
        SEM_TEXT,
        SEM_DATA,
        SEM_STACK,
        SEM_MESSAGE,
        SEM_MATRIX,
        SEM_MAX
};

/* Semantic descriptor associated with a memory region */
struct semantic_descriptor {
        enum semantic_domain domain; /* domain identifier */
        uint32_t attributes;         /* optional attributes */
        uint32_t version;            /* version counter */
};

struct semantic_entry {
        vm_offset_t               start;
        vm_offset_t               end;
        struct semantic_descriptor desc;
        struct semantic_entry    *next;
};

void   vm_semantic_init(void);
int    vm_semantic_register(vm_offset_t start, vm_offset_t end,
            enum semantic_domain dom, uint32_t attr);
struct semantic_entry *vm_semantic_lookup(vm_offset_t start,
            vm_offset_t end);
int    semantic_check_protection(struct semantic_descriptor *desc,
            vm_prot_t prot);

#endif /* _VM_SEMANTIC_H_ */
