/*	$NetBSD: stdarg.h,v 1.15 1999/01/22 14:14:32 mycroft Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)stdarg.h	8.1 (Berkeley) 6/10/93
 */

#ifndef _I386_STDARG_H_
#define	_I386_STDARG_H_

#if defined(MOCK_HAL_ENV_STANDALONE) || defined(TEST_HAL_PCID_ALLOCATOR) || defined(STANDALONE_INTEGRATION_TEST) || defined(VM_SEMANTIC_FSM_STANDALONE) || defined(VM_TEST_SEMANTIC_TRANSITIONS_STANDALONE) || defined(VM_COHERENCE_STANDALONE) || defined(VM_SEMANTIC_NOTIFY_STANDALONE) || defined(VM_SEMANTIC_VALIDATORS_STANDALONE)
// In these standalone modes, this header should be a no-op.
// The C files themselves should include the system <stdarg.h> if needed,
// or it will be pulled in by other system headers like <stdio.h>.
// This prevents this file from shadowing the system's stdarg.h.
#else
// For a kernel build, include the kernel's <sys/stdarg.h>
#include <sys/stdarg.h>
#endif

#endif /* !_I386_STDARG_H_ */
