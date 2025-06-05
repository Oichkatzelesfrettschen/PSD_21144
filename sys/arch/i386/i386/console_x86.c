#include <sys/types.h>
#include <sys/lock.h> // Use lock_object from sys/lock.h
#include <machine/x86_console.h> // Our new internal header
#include <machine/x86_utils.h>   // For x86_outb (if used for direct hardware access)

#include <stdio.h> // For putchar/printf in stubs, replace with actual I/O or kernel printf

// Console state
static struct {
    struct lock_object lock; // Use struct lock_object
    int locking_enabled; // To enable locking after scheduler is up
    // Add other state like current cursor position, video memory address etc. if needed
} x86_console_device;


void x86_cons_init(void) {
    // Initialize console hardware, spinlocks etc.
    simple_lock_init(&x86_console_device.lock, "x86cons"); // Use simple_lock_init
    x86_console_device.locking_enabled = 0; // Locking usually enabled later

    // Actual hardware init (e.g., VGA mode, serial port setup) would go here.
    printf("STUB: x86_cons_init called. Lock initialized. Locking initially disabled.\n");
}

void x86_cons_early_putc(int c) {
    // This version must not use locks and should be callable very early.
    // Direct hardware access (e.g., VGA buffer, polling serial port).
    // For now, using host's putchar for simulation.
    // In a real kernel, this must be replaced with direct hardware access.
    putchar(c);
    if (c == '\n') fflush(stdout); // For test visibility if using host stdio
}

void x86_cons_putc(int c) {
    int use_lock = x86_console_device.locking_enabled; // Check if locking is active

    if (use_lock) {
        simple_lock(&x86_console_device.lock); // Use simple_lock
    }

    // Actual char output logic (e.g., to VGA or serial using x86_outb)
    // This could be identical to early_putc if early version is already hardware-based,
    // or it could be a more advanced version (e.g. interrupt-driven serial).
    x86_cons_early_putc(c); // For this stub, just call the early version.

    if (use_lock) {
        simple_unlock(&x86_console_device.lock); // Use simple_unlock
    }
}

int x86_cons_getc(void) {
    // Poll keyboard controller or serial port. Potentially uses locking.
    // For now, stub: no input
    return -1;
}
