/*	$NetBSD: stdint.h,v 1.8 2018/11/06 16:26:44 maya Exp $	*/

/*-
 * Copyright (c) 2001, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYS_STDINT_H_
#define _SYS_STDINT_H_

#include <sys/cdefs.h>
// <machine/types.h> is included by sys/types.h already if not STANDALONE_INTEGRATION_TEST.
// If it's STANDALONE_INTEGRATION_TEST, sys/types.h includes system <stdint.h> which is what we want.

#if !defined(STANDALONE_INTEGRATION_TEST)
// These definitions will conflict with system <stdint.h> if used in standalone integration tests.
// For kernel builds or specific module standalone tests that *don't* use system stdio/stdlib, these are fine.
#include <machine/types.h> // Provides __intN_t etc. for these typedefs in kernel mode

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

// intptr_t and uintptr_t are C99 types, should come from system <stdint.h> in standalone.
// The _BSD_INTPTR_T_ guards are commented out in the original, suggesting they are expected from elsewhere.
// For kernel, these might be based on __intptr_t from machine/types.h.
typedef	__intptr_t			intptr_t;
typedef	__uintptr_t			uintptr_t;

#ifndef _BSD_REGISTER_T_
typedef	__register_t 		register_t; // __register_t is machine specific
#define _BSD_REGISTER_T_
#endif

#ifndef _BSD_UREGISTER_T_
typedef	__uregister_t 		uregister_t; // __uregister_t is machine specific
#define _BSD_UREGISTER_T_
#endif

#endif // !STANDALONE_INTEGRATION_TEST

#endif /* !_SYS_STDINT_H_ */
