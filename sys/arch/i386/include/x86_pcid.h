#ifndef _X86_PCID_H_
#define _X86_PCID_H_

#include <sys/types.h> // For uint32_t, uint64_t
struct proc; // Forward declaration

void x86_pcid_init(void);
uint64_t x86_allocate_pcid(struct proc *p);
void x86_release_pcid(uint32_t pcid_val);
uint32_t x86_get_pcid_generation(void);

#endif /* _X86_PCID_H_ */
