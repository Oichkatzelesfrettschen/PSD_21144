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

#include <sys/lock.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/token.h>

#include <vm/include/vm_extern.h>
#include <vm/vm_compat.h>

#ifdef OVERLAY
#include <ovl/include/ovl_extern.h>
#endif

/*
 * kern_fork_compat - Fork adapter preserving legacy overlay behaviour.
 */

/*
 * Serialize overlay manipulation during fork to prevent races while the
 * address space is temporarily unmapped.
 */
static struct lwkt_token kf_token;
static int				 kf_token_inited;

void kern_fork_compat_init(void) {
	lwkt_token_init(&kf_token, "kern_fork_compat");
	kf_token_inited = 1;
}

int kern_fork_compat(struct proc* parent, struct proc* child, int isvfork) {
	int error;

	/* Assume the token has been initialised by system startup. */
	if (!kf_token_inited)
		kern_fork_compat_init();

	/* Ensure exclusive access. */
	lwkt_gettoken(&kf_token);

#ifdef OVERLAY
	/* Detach overlays from parent during duplication. */
	ovlspace_mapout(parent->p_ovlspace);
#endif

	/* Duplicate the parent's vmspace.  Return error if allocation fails. */
	child->p_vmspace = vmspace_fork(parent->p_vmspace);
	if (child->p_vmspace == NULL) {
		lwkt_reltoken(&kf_token);
		return ENOMEM;
	}

#ifdef OVERLAY
	/* Attach overlays to the new child. */
	ovlspace_mapin(child->p_ovlspace);
#endif

	/* Let the machine-dependent layer finish the fork. */
	error = cpu_fork(parent, child);

	/* Release lock after fork setup completes. */
	lwkt_reltoken(&kf_token);

	return error;
}
