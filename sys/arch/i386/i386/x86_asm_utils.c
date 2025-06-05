#include <machine/x86_utils.h> // Our new header
#include <sys/types.h>         // For uintN_t types used in function signatures
#include <stdio.h>             // For printf in stubs if inline asm disabled (should be kernel printf)

void x86_enable_interrupts_asm(void) {
#if defined(__GNUC__) || defined(__clang__)
    __asm__ volatile ("sti");
#else
    printf("STUB: x86_enable_interrupts_asm (sti)\n");
#endif
}

void x86_disable_interrupts_asm(void) {
#if defined(__GNUC__) || defined(__clang__)
    __asm__ volatile ("cli");
#else
    printf("STUB: x86_disable_interrupts_asm (cli)\n");
#endif
}

int x86_interrupts_enabled_asm(void) {
#if defined(__GNUC__) || defined(__clang__)
    uintptr_t flags; // Changed from uint32_t to uintptr_t for wider compatibility (e.g. x86_64 if this code is shared)
    __asm__ volatile ("pushf; pop %0" : "=r"(flags));
    return (flags & (1 << 9)) ? 1 : 0; // Check IF bit (Interrupt Flag)
#else
    printf("STUB: x86_interrupts_enabled_asm, returning 0 (false)\n");
    return 0; // Assume disabled if no asm
#endif
}

void x86_cpu_halt_asm(void) {
#if defined(__GNUC__) || defined(__clang__)
    __asm__ volatile ("hlt");
#else
    printf("STUB: x86_cpu_halt_asm (hlt) - entering infinite loop\n");
    // Basic busy loop for stub if hlt is not available/working
    while(1) {}
#endif
}

void x86_cpu_reboot_asm(void) {
    printf("STUB: x86_cpu_reboot_asm (e.g. attempting keyboard controller reset)\n");
#if defined(__GNUC__) || defined(__clang__)
    // Try keyboard controller reset (common method)
    // This is a real operation, be careful if running on bare metal.
    // In a VM, it usually works.
    unsigned char temp;
    do {
        temp = *((volatile unsigned char*)0x64); // Read status port
        if ((temp & 0x01) != 0) { // Check if input buffer empty
            *((volatile unsigned char*)0x60); // Read data port to clear it
        }
    } while ((temp & 0x02) != 0); // Loop if output buffer full
    *((volatile unsigned char*)0x64) = 0xFE; // Pulse reset line

    // If that fails, try triple fault (very disruptive)
    // struct { uint16_t limit; uint64_t base; } idtr_null = {0, 0};
    // __asm__ volatile ("lidt %0; int3" : : "m"(idtr_null));
#endif
    // Fallback: loop indefinitely if other methods don't work or are disabled
    printf("STUB: x86_cpu_reboot_asm fallback loop\n");
    while(1) {}
}
