/*
 * The 3-Clause BSD License:
 * Copyright (c) 2024 The PSD_21144 Contributors
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYS_VM_COMPAT_H_
#define _SYS_VM_COMPAT_H_

#ifndef VM_COMPAT_STANDALONE
#include <sys/proc.h>
#include <vm/include/vm.h>
#endif

/*
 * Macros translating historic proc fields to the new vmspace layout.
 */
#define PROC_TO_VM(p)                 ((p)->p_vmspace)
#define PROC_DATA_SIZE(p)             ((p)->p_vmspace->vm_psegment.ps_dsize)
#define PROC_STACK_SIZE(p)            ((p)->p_vmspace->vm_psegment.ps_ssize)
#define PROC_TEXT_SIZE(p)             ((p)->p_vmspace->vm_psegment.ps_tsize)
#define PROC_DATA_ADDR(p)             ((p)->p_vmspace->vm_psegment.ps_daddr)
#define PROC_STACK_ADDR(p)            ((p)->p_vmspace->vm_psegment.ps_saddr)
#define PROC_TEXT_ADDR(p)             ((p)->p_vmspace->vm_psegment.ps_taddr)
#define PROC_MIN_STACK_ADDR(p)        ((p)->p_vmspace->vm_psegment.ps_minsaddr)
#define PROC_MAX_STACK_ADDR(p)        ((p)->p_vmspace->vm_psegment.ps_maxsaddr)

#endif /* _SYS_VM_COMPAT_H_ */
