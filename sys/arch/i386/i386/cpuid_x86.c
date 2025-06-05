#include <sys/types.h> // For uint32_t etc. - This should ideally come from stdint.h for general code
                       // but for kernel code, sys/types.h is standard.
#include <machine/x86_cpuid.h> // Our new internal header
#include <stdio.h>     // For printf in stubs (if fallback cpuid is used)

// Basic CPUID wrapper.
void x86_cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    if (!eax || !ebx || !ecx || !edx) return;
#if defined(__GNUC__) || defined(__clang__)
    __asm__ volatile (
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(subleaf)
        : "memory" // clobber memory to prevent reordering around cpuid
    );
#else
    // Fallback for other compilers or if inline asm is disabled during initial setup
    *eax = 0; *ebx = 0; *ecx = 0; *edx = 0;
    if (leaf == 0x00000000 && subleaf == 0) { // Max leaf
        *eax = 0x0000000D; // Example: Support up to leaf 0xD
        // Vendor string parts
        *ebx = 0x756e6547; // "Genu"
        *edx = 0x49656e69; // "ineI"
        *ecx = 0x6c65746e; // "ntel"
    } else if (leaf == 0x00000001 && subleaf == 0) { // Features
        *eax = 0x000306A9; // Example Stepping/Model/Family/Type
        *ebx = 0x00010800; // Example Brand Index/CLFLUSH/Max APIC IDs
        *ecx = (1 << 17) | (1<<28); // Example: PCID (bit 17), AVX (bit 28) present
        *edx = (1 << 0) | (1 << 4) | (1 << 23); // Example: FPU, MSR, MMX present
    } else if (leaf == 0x80000000 && subleaf == 0) { // Max extended leaf
        *eax = 0x80000008; // Example: Support up to leaf 0x80000008
    } else if (leaf == 0x80000008 && subleaf == 0) { // Address sizes
        *eax = (48 << 8) | 48; // Example: 48-bit virtual, 48-bit physical
    }
    // Add more stubs for common leaves if needed for HAL testing without real CPUID
    // printf("STUB: x86_cpuid called (leaf=0x%x, subleaf=0x%x). Using stubbed values.\n", leaf, subleaf);
#endif
}

// Example helper, can be expanded
int x86_has_feature_pcid(void) {
    uint32_t eax, ebx, ecx, edx;
    x86_cpuid(0x01, 0, &eax, &ebx, &ecx, &edx);
    return (ecx & (1 << 17)) ? 1 : 0;
}

void x86_get_addr_bits(int *phys, int *virt) {
    uint32_t eax, ebx, ecx, edx;
    if (!phys || !virt) return;

    x86_cpuid(0x80000000, 0, &eax, &ebx, &ecx, &edx);
    if (eax >= 0x80000008) {
        x86_cpuid(0x80000008, 0, &eax, &ebx, &ecx, &edx);
        *phys = eax & 0xFF;
        *virt = (eax >> 8) & 0xFF;
        if (*phys == 0) *phys = 32; // Default if not reported or 0 (unlikely for this leaf)
        if (*virt == 0) *virt = 32;
    } else {
        // Fallback if leaf 0x80000008 is not supported
        *phys = 32; // Default for older CPUs
        *virt = 32;
    }
}
