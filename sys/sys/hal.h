#ifndef _SYS_HAL_H_
#define _SYS_HAL_H_

#include <sys/types.h> // For uintptr_t, int, etc.
// Forward declare structs that the HAL interface might need.
// Actual definitions would be in other headers like sys/proc.h, sys/mmu.h (or arch-specific versions)
struct proc;
struct pde; // Placeholder for Page Directory Entry type - actual type from mmu.h
struct trapframe; // Placeholder

// Memory model enumeration
enum hal_memory_model {
    MEM_MODEL_UNKNOWN = 0,
    MEM_MODEL_TSO,       // Total Store Order (e.g., x86)
    MEM_MODEL_PSO,       // Partial Store Order
    MEM_MODEL_RELAXED    // Relaxed (e.g., some ARM)
};

// Structure to hold capabilities reported by the HAL
struct hal_capabilities {
    int has_cpuid;        // CPUID instruction available
    int has_pcid;         // Process Context Identifiers support
    int addr_bits_phys;   // Physical address bits
    int addr_bits_virt;   // Virtual address bits
    enum hal_memory_model memory_model;
    // Add other generic capabilities as needed, e.g.:
    // int has_fpu;
    // int has_apic;
    // int num_cores;
};

// --- HAL Initialization ---
void hal_early_init(void); // Early hardware setup, console may not be ready
void hal_init(void);       // Main HAL initialization, console should be working

// --- Console I/O ---
void hal_console_early_putc(int c); // For very early messages, may be polling
void hal_console_putc(int c);
int hal_console_getc(void);
void hal_console_gets(char *buf, int len); // Optional: buffered line input

// --- CPU Management ---
void hal_cpu_get_capabilities(struct hal_capabilities *cap);
void hal_cpu_halt(void);
void hal_cpu_reboot(void);
void hal_cpu_enable_interrupts(void);
void hal_cpu_disable_interrupts(void);
int  hal_cpu_interrupts_enabled(void);
// const char* hal_cpu_get_arch_string(void); // e.g., "i386", "amd64"

// --- MMU / Address Space Management ---
// These are highly abstract; actual params depend on MMU design.
// Using 'uintptr_t' for addresses and 'struct pde*' as an opaque type for page directories.
// The real types (paddr_t, vaddr_t, pgd_t/pde_t) would come from kernel's mmu.h/types.h.

// Example: Switch current address space (pgdir = page directory pointer)
void hal_mmu_switch_as(struct proc *p, uintptr_t pgdir_phys);
// Example: Get physical address for a virtual address in a given address space
uintptr_t hal_mmu_va_to_pa(uintptr_t pgdir_phys, uintptr_t va);
// Example: Create a new address space
uintptr_t hal_mmu_create_as(void); // Returns physical address of new top-level page table
// Example: Destroy an address space
void hal_mmu_destroy_as(uintptr_t pgdir_phys);
// Example: Map a page
int hal_mmu_map_page(uintptr_t pgdir_phys, uintptr_t va, uintptr_t pa, int prot_flags);
// Example: Unmap a page
void hal_mmu_unmap_page(uintptr_t pgdir_phys, uintptr_t va);
// Example: TLB flush operations
void hal_mmu_flush_tlb_full(void);
void hal_mmu_flush_tlb_asid(uintptr_t asid_handle); // ASID or pgdir handle
void hal_mmu_flush_tlb_va(uintptr_t asid_handle, uintptr_t va);

// --- Timer ---
void hal_timer_init(unsigned int frequency); // Program system timer
uint64_t hal_timer_get_ticks(void);        // Get current tick count
void hal_timer_set_alarm(uint64_t ticks);   // Set a one-shot alarm

// --- Interrupts ---
// void hal_intr_init(void); // Initialize interrupt controllers
// int hal_intr_register(int irq, void (*handler)(struct trapframe *tf, int irq));
// void hal_intr_unregister(int irq);
// void hal_intr_enable(int irq);
// void hal_intr_disable(int irq);
// void hal_intr_eoi(int irq); // End Of Interrupt for level-triggered

// --- Spinlocks (basic HAL-level might be needed for its own internals) ---
// typedef ... hal_spinlock_t;
// void hal_spinlock_init(hal_spinlock_t *lock, const char *name);
// void hal_spinlock_acquire(hal_spinlock_t *lock);
// void hal_spinlock_release(hal_spinlock_t *lock);


// Global HAL operations table / function pointers (optional, an alternative to direct linking)
// struct hal_ops {
//    void (*cpu_reboot)(void);
//    ...
// };
// extern struct hal_ops hal_ops_table;


#endif /* _SYS_HAL_H_ */
