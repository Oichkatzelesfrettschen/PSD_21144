#ifndef _X86_UTILS_H_
#define _X86_UTILS_H_

#include <sys/types.h> // For uintN_t types

void x86_enable_interrupts_asm(void);
void x86_disable_interrupts_asm(void);
int  x86_interrupts_enabled_asm(void);
void x86_cpu_halt_asm(void);
void x86_cpu_reboot_asm(void);

// Basic I/O port operations
uint8_t x86_inb(uint16_t port);
void x86_outb(uint16_t port, uint8_t data);
uint16_t x86_inw(uint16_t port);
void x86_outw(uint16_t port, uint16_t data);
uint32_t x86_inl(uint16_t port);
void x86_outl(uint16_t port, uint32_t data);


#endif /* _X86_UTILS_H_ */
