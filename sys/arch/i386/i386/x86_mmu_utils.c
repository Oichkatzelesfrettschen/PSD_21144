#include <sys/types.h> // For uintptr_t. Should be okay in kernel builds.
                       // For standalone test builds, types.h needs to be minimal or stdint used.
#include <machine/x86_mmu_utils.h> // Our new header
#include <stdio.h>     // For printf in stubs (should be replaced by kprintf in real kernel code)


uintptr_t x86_mmu_kernel_pgdir_phys = 0; // Placeholder for kernel page directory phys addr
                                       // Should be initialized by pmap_bootstrap or similar.

uintptr_t x86_rcr3(void) {
#if defined(__GNUC__) || defined(__clang__)
    uintptr_t cr3_val;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3_val));
    return cr3_val;
#else
    printf("STUB: x86_rcr3 called, returning 0 (inline asm disabled)\n");
    return 0;
#endif
}

void x86_invlpg(uintptr_t va) {
#if defined(__GNUC__) || defined(__clang__)
    __asm__ volatile ("invlpg (%0)" : : "r"(va) : "memory");
#else
    printf("STUB: x86_invlpg for va 0x%lx (inline asm disabled)\n", (unsigned long)va);
#endif
}

// Renamed from x86_uva2pa to match header
uintptr_t x86_mmu_get_pa(uintptr_t pgdir_phys, uintptr_t va) {
    // This is a highly simplified stub.
    // A real implementation would walk page tables starting from pgdir_phys.
    if (pgdir_phys == x86_mmu_kernel_pgdir_phys && va < 0x1000000) {
        return va;
    }
    printf("STUB: x86_mmu_get_pa (pgdir_phys=0x%lx, va=0x%lx) returning 0 (not found/error or not simple identity map)\n",
           (unsigned long)pgdir_phys, (unsigned long)va);
    return 0;
}

void x86_lcr3(uintptr_t cr3_val) {
#if defined(__GNUC__) || defined(__clang__)
    __asm__ volatile ("mov %0, %%cr3" : : "r"(cr3_val) : "memory");
#else
    printf("STUB: x86_lcr3 called with 0x%lx (inline asm disabled)\n", (unsigned long)cr3_val);
#endif
}

void x86_lcr4(uintptr_t cr4_val) {
#if defined(__GNUC__) || defined(__clang__)
    __asm__ volatile ("mov %0, %%cr4" : : "r"(cr4_val) : "memory");
#else
    printf("STUB: x86_lcr4 called with 0x%lx (inline asm disabled)\n", (unsigned long)cr4_val);
#endif
}

uintptr_t x86_rcr4(void) {
#if defined(__GNUC__) || defined(__clang__)
    uintptr_t cr4_val;
    __asm__ volatile ("mov %%cr4, %0" : "=r"(cr4_val));
    return cr4_val;
#else
    printf("STUB: x86_rcr4 called, returning 0 (inline asm disabled)\n");
    return 0;
#endif
}
