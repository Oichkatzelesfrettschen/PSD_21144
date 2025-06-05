#ifndef _X86_MMU_UTILS_H_
#define _X86_MMU_UTILS_H_

#include <sys/types.h> // For uintptr_t

// Functions that directly manipulate MMU registers or arch-specific structures
void x86_lcr3(uintptr_t cr3_val);
void x86_lcr4(uintptr_t cr4_val);
uintptr_t x86_rcr4(void);
uintptr_t x86_rcr3(void); // Added for completeness if needed for TLB flushes
void x86_invlpg(uintptr_t va);

// x86 specific VA to PA conversion (raw, might be different from HAL version)
// pgdir_phys is physical address of page directory
uintptr_t x86_mmu_get_pa(uintptr_t pgdir_phys, uintptr_t va);

#endif // _X86_MMU_UTILS_H_
