#ifndef _MACHINE_PARAM_H_
#define _MACHINE_PARAM_H_

// This is a dummy file for standalone compilation to satisfy #include <machine/param.h>
// from sys/param.h.

// It would normally define machine-dependent kernel parameters like HZ, tick, PAGE_SIZE etc.
// For a standalone build, these might not be strictly needed unless the code
// being tested uses them.

// Example definitions (values may not be accurate or relevant for all tests):
#define HZ 100
#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define MAXPHYS (64 * 1024)   /* max raw I/O transfer size */
// #define CLSIZE		1	/* number of pages per CLBYTES */ /* Usually derived */
// #define CLBYTES		(CLSIZE*PAGE_SIZE)

#endif /* _MACHINE_PARAM_H_ */
