#ifndef _MACHINE_SIGNAL_H_
#define _MACHINE_SIGNAL_H_

// This is a dummy file for standalone compilation to satisfy #include <machine/signal.h>
// from sys/signal.h (which is included by sys/param.h).

// Kernel signal definitions are complex and machine-dependent.
// For a standalone build not testing signals, this can be minimal.

// struct sigcontext { /* machine-dependent signal context */ };
// typedef int sig_atomic_t; // Usually in <signal.h>

#endif /* _MACHINE_SIGNAL_H_ */
