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

/*
 * Preemptive scheduler stubs.
 * This module provides minimal hooks for cooperative preemption.
 */

#ifdef SCHED_PREEMPT_STANDALONE
struct proc {
	int dummy;
};
typedef int		   simple_lock_data_t;
static inline void simple_lock_init(simple_lock_data_t* l, const char* n) {
	(void) l;
	(void) n;
}
static inline void simple_lock(simple_lock_data_t* l) {
	(void) l;
}
static inline void simple_unlock(simple_lock_data_t* l) {
	(void) l;
}
void setrunnable(struct proc*);
#else
#include <sys/lock.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/systm.h>
#endif

/* lock protecting preemption operations */
static simple_lock_data_t sched_preempt_lock;
static int				  sched_preempt_inited;

/*
 * Initialize the preemption lock on first use.
 */
void sched_preempt_init(void) {
	simple_lock_init(&sched_preempt_lock, "sched_preempt");
	sched_preempt_inited = 1;
}

/*
 * sched_preempt - request a context switch if needed.
 * The implementation simply marks the process runnable under lock.
 */
void sched_preempt(struct proc* p) {
	if (!sched_preempt_inited)
		sched_preempt_init();
	simple_lock(&sched_preempt_lock);
	setrunnable(p);
	simple_unlock(&sched_preempt_lock);
}
