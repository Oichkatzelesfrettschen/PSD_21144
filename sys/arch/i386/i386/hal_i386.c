#include <sys/types.h>  // For basic types
#include <sys/types.h> // Should be included by other headers if they need base types.
#include <sys/param.h>  // For system parameters (e.g. HZ)
#include <sys/hal.h>    // The HAL interface we are implementing
#include <sys/proc.h>   // For struct proc
#include <sys/systm.h>  // For kprintf, panic

// Include new internal headers
#include <machine/x86_console.h>
#include <machine/x86_cpuid.h>
#include <machine/x86_mmu_utils.h>
#include <machine/x86_utils.h>
#include <machine/x86_pcid.h>     // For PCID functions
#include <machine/specialreg.h> // For CR4_PCIDE


// Global variable from x86_mmu_utils.c
extern uintptr_t x86_mmu_kernel_pgdir_phys;

// CRITICAL STUB: V2P (Virtual to Physical address conversion)
// This is a placeholder and needs a real implementation from the pmap module.
#ifndef V2P
#define V2P(va) ((uintptr_t)(va)) // Incorrect: Assumes virtual == physical for stubbing
#endif

// CRITICAL STUB: myproc (get current process pointer)
// This is a placeholder and needs to be provided by the scheduler/process management.
static struct proc *myproc_stub = NULL;
#define myproc() (myproc_stub)

// --- HAL Implementation ---

void hal_early_init(void) {
    // Very early hardware setup, e.g., minimal console
    // x86_cons_early_init(); // If exists
}

void hal_init(void) {
    x86_cons_init(); // Initialize console fully
    x86_pcid_init(); // Initialize PCID state

    struct hal_capabilities caps;
    hal_cpu_get_capabilities(&caps);
    if (caps.has_pcid) {
        uintptr_t cr4 = x86_rcr4();
        if (!(cr4 & CR4_PCIDE)) {
            kprintf("HAL_INIT: CPU supports PCID, enabling CR4.PCIDE.\n");
            x86_lcr4(cr4 | CR4_PCIDE);
        } else {
            kprintf("HAL_INIT: CR4.PCIDE already enabled.\n");
        }
    } else {
        kprintf("HAL_INIT: CPU does not support PCID.\n");
    }
    kprintf("HAL: i386 HAL initialized.\n");
}

void hal_console_early_putc(int c) {
    x86_cons_early_putc(c);
}

void hal_console_putc(int c) {
    x86_cons_putc(c);
}

int hal_console_getc(void) {
    return x86_cons_getc();
}

void hal_console_gets(char *buf, int len) {
    // Basic stub, real implementation would need input buffering, echo etc.
    if (len <= 0) return;
    // For now, simulate no input or a fixed string for testing if needed
    // buf[0] = '\0';
    (void)buf; (void)len; // Suppress unused
}


void hal_cpu_get_capabilities(struct hal_capabilities *cap) {
    if (!cap) return;

    cap->has_cpuid = 1;

    uint32_t eax, ebx, ecx, edx;
    int p_addr_bits = 32, v_addr_bits = 32;

    x86_cpuid(0x01, 0, &eax, &ebx, &ecx, &edx);
    cap->has_pcid = (ecx & (1 << 17)) ? 1 : 0;

    x86_cpuid(0x80000000, 0, &eax, &ebx, &ecx, &edx);
    if (eax >= 0x80000008) {
        x86_cpuid(0x80000008, 0, &eax, &ebx, &ecx, &edx);
        p_addr_bits = eax & 0xFF;
        v_addr_bits = (eax >> 8) & 0xFF;
        if (p_addr_bits == 0) p_addr_bits = 32;
        if (v_addr_bits == 0) v_addr_bits = 32;
    }
    cap->addr_bits_phys = p_addr_bits;
    cap->addr_bits_virt = v_addr_bits;

    cap->memory_model = MEM_MODEL_TSO;
}

void hal_cpu_halt(void) {
    x86_disable_interrupts_asm();
    for (;;) {
        x86_cpu_halt_asm();
    }
}

void hal_cpu_reboot(void) {
    x86_cpu_reboot_asm();
}

void hal_cpu_enable_interrupts(void) {
    x86_enable_interrupts_asm();
}

void hal_cpu_disable_interrupts(void) {
    x86_disable_interrupts_asm();
}

int hal_cpu_interrupts_enabled(void) {
    return x86_interrupts_enabled_asm();
}

void hal_mmu_switch_as(struct proc *p, uintptr_t pgdir_phys) { // Changed to pgdir_phys
    uintptr_t pgdir_phys_to_load;

    if (!p) {
        if (pgdir_phys != 0) { // Use argument name
             x86_lcr3(pgdir_phys);
        } else {
             panic("hal_mmu_switch_as: NULL proc and no override pgdir");
        }
        return;
    }

    // Determine physical page directory to load
    if (p->p_paddr_pgdir != 0) {
        pgdir_phys_to_load = p->p_paddr_pgdir;
    } else if (pgdir_phys != 0) { // Use argument name
        pgdir_phys_to_load = pgdir_phys;
    } else {
        panic("hal_mmu_switch_as: No page directory physical address for proc PID %d", p->p_pid);
        return;
    }

    struct hal_capabilities caps;
    hal_cpu_get_capabilities(&caps);

    if (caps.has_pcid) {
        uintptr_t cr4 = x86_rcr4();
        if (!(cr4 & CR4_PCIDE)) {
            kprintf("HAL_WARN: hal_mmu_switch_as: PCID capable CPU but CR4.PCIDE is off!\n");
            x86_lcr3(pgdir_phys_to_load);
            return;
        }

        uint64_t stored_asid = p->p_arch_asid;
        uint32_t current_gen = x86_get_pcid_generation();
        uint32_t proc_gen = (uint32_t)(stored_asid >> 32);
        uint32_t pcid = (uint32_t)(stored_asid & 0xFFF);

        if (proc_gen != current_gen || pcid == 0) {
            stored_asid = x86_allocate_pcid(p);
            p->p_arch_asid = stored_asid;
            pcid = (uint32_t)(stored_asid & 0xFFF);
            // kprintf("HAL: Allocated new PCID %u (Gen %u) for PID %d\n", pcid, (uint32_t)(stored_asid >> 32), p->p_pid);
        }

        uintptr_t cr3_value = pgdir_phys_to_load | pcid;
        // The NOFLUSH bit (bit 63) logic has been removed as per subtask instruction.
        // Standard CR3 load with PCID handles TLB entries based on PCID matching.
        x86_lcr3(cr3_value);
    } else {
        // Fallback to non-PCID CR3 load
        x86_lcr3(pgdir_phys_to_load);
    }
}

uintptr_t hal_mmu_va_to_pa(uintptr_t pgdir_phys, uintptr_t va) { // Changed to pgdir_phys
    return x86_mmu_get_pa(pgdir_phys, va);
}

uintptr_t hal_mmu_create_as(void) {
#ifdef HAL_I386_STANDALONE_TEST
    printf("STUB: hal_mmu_create_as called\n");
#endif
    return 0;
}
void hal_mmu_destroy_as(uintptr_t pgdir_phys) { (void)pgdir_phys; /* Stub */ }
int hal_mmu_map_page(uintptr_t pgdir_phys, uintptr_t va, uintptr_t pa, int prot_flags) { (void)pgdir_phys; (void)va; (void)pa; (void)prot_flags; return -1; /* Stub */ }
void hal_mmu_unmap_page(uintptr_t pgdir_phys, uintptr_t va) { (void)pgdir_phys; (void)va; /* Stub */ }

void hal_mmu_flush_tlb_full(void) {
    // Reload CR3 to flush the entire TLB (non-PCID aware part)
    // For PCID systems, one might need to invalidate all entries or use INVPCID.
    // This simple CR3 reload is a common way.
    uintptr_t current_cr3 = x86_rcr3(); // Get current CR3
    x86_lcr3(current_cr3);              // Write it back
}

void hal_mmu_flush_tlb_asid(uintptr_t asid_handle) {
    (void)asid_handle; // Stub - a real implementation would use INVPCID or reload CR3
                       // if ASID is part of CR3 and PCIDE is enabled in CR4.
                       // For now, can also just do a full flush as a basic measure.
    hal_mmu_flush_tlb_full();
}

void hal_mmu_flush_tlb_va(uintptr_t asid_handle, uintptr_t va) {
    (void)asid_handle; // asid_handle might be used with INVPCID type ALL
    x86_invlpg(va);      // Invalidate a single page translation
}

// --- Timer Stubs ---
void hal_timer_init(unsigned int frequency) { (void)frequency; /* Stub */ }
uint64_t hal_timer_get_ticks(void) { return 0; /* Stub */ }
void hal_timer_set_alarm(uint64_t ticks) { (void)ticks; /* Stub */ }

// Note: printf calls in this file are for debugging if compiled in a test harness.
// In a real kernel build, they would be replaced by kernel logging functions (e.g., kprintf).
// The HAL_I386_STANDALONE_TEST guard is an example if this file itself were made testable.
// For normal kernel builds, these printf calls should be excluded.
// The current subtask does not require making this file testable standalone.
