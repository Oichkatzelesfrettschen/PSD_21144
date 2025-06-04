/*
 * The 3-Clause BSD License:
 * Copyright (c) 2024-2025 The PSD_21144 Contributors
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
 * Unified signal delivery across virtualization levels.  This module
 * associates each signal with a semantic domain and supports optional
 * zero-copy delivery for small messages.
 */

#ifdef SIG_UNIFIED_STANDALONE
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
typedef unsigned long vm_offset_t;
typedef void (*sig_t)(int);
struct sigframe { int dummy; };
typedef unsigned long u_long;
struct proc { int dummy; };
struct lwkt_token { int t; };
static inline void lwkt_token_init(struct lwkt_token *t, const char *n)
{ (void)t; (void)n; }
static inline void lwkt_gettoken(struct lwkt_token *t)
{ (void)t; }
static inline void lwkt_reltoken(struct lwkt_token *t)
{ (void)t; }
#define _SIG_MAXSIG 32
#define NUM_SEMANTIC_DOMAINS 4
#else
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/token.h>
#include <vm/vm_compat.h>
#endif

/*
 * Additional information pushed onto the user stack when a signal is
 * delivered through the unified interface.
 */
struct semantic_signal_frame {
        struct sigframe sf_base;          /* traditional signal frame */
        uint32_t        sf_semantic_domain;/* domain interrupted */
        uint32_t        sf_virt_level;    /* virtualization level */
        uint64_t        sf_semantic_pc;   /* pc within semantic space */
        vm_offset_t     sf_shared_base;   /* shared buffer base */
        size_t          sf_shared_size;   /* shared buffer size */
};

/* One token per signal and semantic domain to serialize delivery. */
static struct lwkt_token sig_tokens[_SIG_MAXSIG][NUM_SEMANTIC_DOMAINS];

/*
 * Initialize the signal tokens during startup.
 */
void
kern_sig_unified_init(void)
{
        int  sig, dom;
        char name[32];

        for (sig = 0; sig < _SIG_MAXSIG; sig++) {
                for (dom = 0; dom < NUM_SEMANTIC_DOMAINS; dom++) {
                        snprintf(name, sizeof(name), "sig_%d_dom_%d", sig, dom);
                        lwkt_token_init(&sig_tokens[sig][dom], name);
                }
        }
}

/*
 * Deliver a signal with semantic awareness.  Only a skeleton of the
 * real implementation is provided for unit testing.
 */
int
semantic_sendsig(sig_t catcher, int sig, int mask, u_long code,
                 int semantic_domain, int virt_level)
{
        struct lwkt_token *token;
        (void)mask;
        (void)virt_level;

        token = &sig_tokens[sig][semantic_domain];
        lwkt_gettoken(token);

        /* real kernel would build a signal frame and copy it out */
        if (catcher != NULL)
                catcher(sig);

        lwkt_reltoken(token);
        return 0;
}
