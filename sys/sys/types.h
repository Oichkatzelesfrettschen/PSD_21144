/*-
 * Copyright (c) 1982, 1986, 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)types.h	8.6 (Berkeley) 2/19/95
 */
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)types.h	1.4.1 (2.11BSD) 2000/2/28
 */

#ifndef _SYS_TYPES_H_
#define	_SYS_TYPES_H_

#if defined(STANDALONE_INTEGRATION_TEST)
// In standalone integration mode, pull in standard types first.
// This helps avoid conflicts with system headers included by stdio, stdlib etc.
#include <stddef.h> // For size_t, ssize_t, ptrdiff_t
#include <stdint.h> // For intN_t, uintN_t, intptr_t, uintptr_t
#include <time.h>   // For time_t, clock_t (if needed, often in sys/types.h)
                    // Note: time.h might not define all BSD versions like _BSD_TIME_T_
                    // but we want the standard ones.
#else // Not STANDALONE_INTEGRATION_TEST (i.e. kernel build or specific module standalone)
/* Machine type dependent parameters. */
#include <machine/ansi.h> // Provides __intN_t, __uintN_t, etc.
#include <machine/types.h>  // Provides other machine specifics if any.

#include <sys/ansi.h>     // Provides _BSD_FOO_T_ style macros and perhaps more types.

// Define standard integer types based on machine-specific __intN_t etc.
// These are guarded by _BSD_INTN_T_ to allow overriding by sys/ansi.h or machine/ansi.h
#ifndef	_BSD_INT8_T_
typedef	__int8_t			int8_t;
#define	_BSD_INT8_T_
#endif
#ifndef	_BSD_UINT8_T_
typedef	__uint8_t			uint8_t;
#define	_BSD_UINT8_T_
#endif
#ifndef	_BSD_INT16_T_
typedef	__int16_t			int16_t;
#define	_BSD_INT16_T_
#endif
#ifndef	_BSD_UINT16_T_
typedef	__uint16_t			uint16_t;
#define	_BSD_UINT16_T_
#endif
#ifndef	_BSD_INT32_T_
typedef	__int32_t			int32_t;
#define	_BSD_INT32_T_
#endif
#ifndef	_BSD_UINT32_T_
typedef	__uint32_t			uint32_t;
#define	_BSD_UINT32_T_
#endif
#ifndef	_BSD_INT64_T_
typedef	__int64_t			int64_t;
#define	_BSD_INT64_T_
#endif
#ifndef	_BSD_UINT64_T_
typedef	__uint64_t			uint64_t;
#define	_BSD_UINT64_T_
#endif
#ifndef	_BSD_INTPTR_T_
typedef	__intptr_t			intptr_t;
#define	_BSD_INTPTR_T_
#endif
#ifndef	_BSD_UINTPTR_T_
typedef	__uintptr_t			uintptr_t;
#define	_BSD_UINTPTR_T_
#endif
#ifndef _BSD_REGISTER_T_
typedef	__register_t 		register_t;
#define _BSD_REGISTER_T_
#endif
#ifndef _BSD_UREGISTER_T_
typedef	__uregister_t 		uregister_t;
#define _BSD_UREGISTER_T_
#endif

// BSD-style unsigned bits types (u_intN_t are not standard C, so usually safe)
typedef	uint8_t				u_int8_t;
typedef	uint16_t			u_int16_t;
typedef	uint32_t			u_int32_t;
typedef	uint64_t			u_int64_t;

#endif // STANDALONE_INTEGRATION_TEST specific type handling


// Common definitions for both kernel and potentially some standalone modes (non-POSIX part)
#ifndef _POSIX_SOURCE
// u_char, u_short, etc. are BSD specific, not standard C.
// Guard them if STANDALONE_INTEGRATION_TEST needs to avoid even these to be pure.
// For now, assume they are okay as they are prefixed.
typedef	unsigned char		u_char;
typedef	unsigned short		u_short;
typedef	unsigned int		u_int;
typedef	unsigned long		u_long;

typedef unsigned char		unchar;		/* Sys III/V compatibility */
typedef	unsigned short		ushort;		/* Sys III/V compatibility */
typedef	unsigned int		uint;		/* Sys III/V compatibility */
typedef unsigned long		ulong;		/* Sys III/V compatibility */
#endif /* _POSIX_SOURCE */

// quad_t and u_quad_t are BSD specific.
// In STANDALONE_INTEGRATION_TEST, uint64_t/int64_t come from <stdint.h>
typedef	uint64_t	 		u_quad_t; 	/* quads */
typedef	int64_t				quad_t;
typedef	quad_t 				*qaddr_t;


// Kernel-specific types or types also used by userspace programs (like ps)
// These are generally okay to define as they are specific to this OS/kernel environment.
#if defined(_KERNEL) || defined(STANDALONE_INTEGRATION_TEST) // Make available for standalone if they are used by our C files
// For STANDALONE_INTEGRATION_TEST, ensure these don't clash with what stddef/stdint might provide if names are too generic.
// The __* versions are usually from machine/ansi.h or machine/types.h for kernel.
// For standalone, we might need to provide dummy __* types if these are used.
// For now, let's assume these are sufficiently kernel-specific.

#ifdef _KERNEL // Only if actually kernel, not general standalone
typedef u_long				fixpt_t;	/* fixed point number */
typedef	u_short				nlink_t;	/* link count */
typedef	long				segsz_t;	/* segment size */
typedef	long				daddr_t;	/* disk address */
typedef	char 				*caddr_t;	/* core address */
typedef	u_long				ino_t;		/* inode number*/
typedef	long				swblk_t;	/* swap offset */
// time_t is standard, handled below
typedef	u_long				dev_t;		/* device number */
// off_t is standard (POSIX), handled below
typedef	u_short				mode_t;		/* permissions */
typedef long				memaddr_t;	/* core & swap address */
typedef u_quad_t			fsblkcnt_t; /* fs block count (statvfs) */
typedef u_quad_t			fsfilcnt_t; /* fs file count */
typedef	u_long				gid_t;		/* group id */
typedef	u_char	    		pid_t;		/* process id - careful with standard definition */
typedef u_char 				pri_t;		/* process priority */
typedef	u_long				uid_t;		/* user id */
typedef uint32_t			in_addr_t;	/* IP(v4) address */
typedef uint16_t			in_port_t;	/* "Internet" port number */

#else // Userland or STANDALONE_INTEGRATION_TEST that needs some of these (often from <sys/stat.h> etc.)
// In STANDALONE_INTEGRATION_TEST, rely on system headers for POSIX types like pid_t, off_t, time_t, mode_t, gid_t, uid_t.
// Define only non-standard kernel types here if needed by the standalone code.
// The __* types are problematic for STANDALONE_INTEGRATION_TEST if they are meant to be defined by our dummy machine/*.h
// Let's assume for STANDALONE_INTEGRATION_TEST, these are not needed from here if they conflict.
#ifndef STANDALONE_INTEGRATION_TEST
typedef __fixpt_t 			fixpt_t;
typedef	__nlink_t 			nlink_t;
typedef	long     			segsz_t;
typedef uint64_t			rlim_t;
typedef __daddr_t			daddr_t;
typedef __caddr_t 			caddr_t;
typedef __ino_t				ino_t;
typedef	__swblk_t 			swblk_t;
typedef __dev_t				dev_t;
typedef __off_t 			off_t;
typedef __mode_t 			mode_t;
typedef __memaddr_t			memaddr_t;
typedef __fsblkcnt_t		fsblkcnt_t;
typedef __fsfilcnt_t		fsfilcnt_t;
typedef __gid_t 			gid_t;
typedef __pid_t 			pid_t;
typedef	__pri_t				pri_t;
typedef __uid_t 			uid_t;
typedef __in_addr_t			in_addr_t;
typedef __in_port_t			in_port_t;
typedef	uint32_t	        id_t;		/* group id, process id or user id */
#endif // !STANDALONE_INTEGRATION_TEST

#endif /* _KERNEL distinction for userspace vs kernel specific types */


#if (defined(_KERNEL) || defined(_LIBC)) && !defined(STANDALONE_INTEGRATION_TEST)
// This sys/stdint.h is likely a kernel-specific stdint, avoid in standalone integration.
#include <sys/stdint.h>
typedef intptr_t 			semid_t;
#endif /* (_KERNEL || _LIBC) && !STANDALONE_INTEGRATION_TEST */

#include <sys/select.h> // fd_set might be okay as it's usually not in std C headers.

/*
 * Basic system types and major/minor device constructing/busting macros.
 */
#define	major(x)			((int)(((int)(x)>>8)&0377))
#define	minor(x)			((int)((x)&0377))
#define	makedev(x,y)		((dev_t)(((x)<<8)|(y)))

#define	NBBY				8

#ifndef howmany
#define	howmany(x, y)		(((x)+((y)-1))/(y))
#endif

#ifndef STANDALONE_INTEGRATION_TEST
#include <machine/endian.h> // For kernel, rely on full machine/endian.h
#else
// For STANDALONE_INTEGRATION_TEST, ensure basic endianness is defined if not by system headers.
// Our dummy machine/endian.h should cover this for our own code.
#include <machine/endian.h>
#endif


#if !defined(STANDALONE_INTEGRATION_TEST)
// Standard C types like size_t, time_t, clock_t should come from
// system headers (<stddef.h>, <time.h>) in standalone integration test mode.
// The _BSD_ guards are for compatibility with other BSD headers.
#ifdef	_BSD_CLOCK_T_
typedef	_BSD_CLOCK_T_		clock_t;
#undef	_BSD_CLOCK_T_
#endif
#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_		size_t;
#undef	_BSD_SIZE_T_
#endif
#ifdef	_BSD_SSIZE_T_
typedef	_BSD_SSIZE_T_		ssize_t;
#undef	_BSD_SSIZE_T_
#endif
#ifdef	_BSD_TIME_T_
typedef	_BSD_TIME_T_		time_t;
#undef	_BSD_TIME_T_
#endif
#endif // !STANDALONE_INTEGRATION_TEST


// These seem more specific BSD types, might be okay, or guard them too if they clash.
#ifndef STANDALONE_INTEGRATION_TEST // Guarding them too for safety initially
#ifdef	_BSD_CLOCKID_T_
typedef	_BSD_CLOCKID_T_		clockid_t;
#undef	_BSD_CLOCKID_T_
#endif
#ifdef	_BSD_TIMER_T_
typedef	_BSD_TIMER_T_		timer_t;
#undef	_BSD_TIMER_T_
#endif
#endif // !STANDALONE_INTEGRATION_TEST


#if defined(_KERNEL) || defined(_STANDALONE) // _STANDALONE here is a general kernel standalone, not our test flag
#define SET(t, f)			((t) |= (f))
#define	ISSET(t, f)			((t) & (f))
#define	CLR(t, f)			((t) &= ~(f))
#endif

#if defined(__STDC__) && defined(_KERNEL)
/*
 * Forward structure declarations for function prototypes.
 */
struct	proc;
struct	pgrp;
struct	ucred;
struct	rusage;
struct	k_rusage;
struct	file;
struct	buf;
struct	tty;
struct	uio;
struct	user;
#endif

#if defined(_KERNEL) || defined(_STANDALONE) // General kernel standalone
// In STANDALONE_INTEGRATION_TEST, stdbool.h should be included by C files directly if needed.
#ifndef STANDALONE_INTEGRATION_TEST
#include <sys/stdbool.h> // This might be a kernel-specific stdbool.h
#else
#include <stdbool.h> // Use standard stdbool for integration tests
#endif

/* 2.11BSD bool_t type. Needed for backwards compatability. */
#ifndef STANDALONE_INTEGRATION_TEST // Avoid if bool is already defined via stdbool.h
typedef char	bool_t;
#endif

/*
 * Deprecated Mach-style boolean_t type.  Should not be used by new code.
 */
#ifndef boolean_t // If not defined by stdbool.h or elsewhere
typedef int		boolean_t;
#endif
#ifndef TRUE
#define	TRUE	1
#endif
#ifndef FALSE
#define	FALSE	0
#endif
#endif /* _KERNEL || _STANDALONE */
#endif /* _SYS_TYPES_H_ */
