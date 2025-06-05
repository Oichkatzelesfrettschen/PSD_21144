#ifndef _X86_CPUID_H_
#define _X86_CPUID_H_

#include <sys/types.h> // For uint32_t

void x86_cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);
// Specific feature checks can also be declared here if they are to be used by other x86 modules.
// For now, hal_i386.c can call x86_cpuid directly and interpret bits.
// int x86_has_feature_pcid(void); // Example from previous hal_i386.c
// void x86_get_addr_bits(int *phys, int *virt); // Example

#endif /* _X86_CPUID_H_ */
