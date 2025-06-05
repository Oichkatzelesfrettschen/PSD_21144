#ifndef _MACHINE_LIMITS_H_
#define _MACHINE_LIMITS_H_

// This is a dummy file for standalone compilation to satisfy #include <machine/limits.h>
// from sys/param.h.

// Standard C limits like INT_MAX etc., should come from <limits.h>
// which should be included by the C files themselves if they use these constants.
// This file typically defines machine-specific limits if they are different
// or extend standard limits. For a generic standalone build, this can be minimal.

// Example:
// #define CHAR_BIT 8
// #define INT_MAX 2147483647
// #define LONG_MAX 9223372036854775807L

// If sys/param.h specifically needs something from here, it would be added.
// For now, assume system <limits.h> (included by C files directly) is preferred
// for standard limit constants in STANDALONE_INTEGRATION_TEST mode.

#endif /* _MACHINE_LIMITS_H_ */
