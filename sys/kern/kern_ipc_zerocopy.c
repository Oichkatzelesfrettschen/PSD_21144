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
 * Zero-copy IPC primitives.  These routines create shared memory
 * channels between processes and use lightweight tokens for
 * synchronization.
 */

#ifdef ZEROCOPY_STANDALONE
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "../vm/include/vm_coherence.h"
#include "../vm/include/vm_semantic.h"
typedef unsigned long vm_offset_t;
typedef unsigned long vm_size_t;
typedef unsigned long vm_ooffset_t;
typedef void *vm_object_t;
struct vm_map { int dummy; };
struct vmspace { struct vm_map vm_map; };
struct proc { struct vmspace *p_vmspace; };
struct lwkt_token { int t; };
static inline void lwkt_token_init(struct lwkt_token *t, const char *n)
{ (void)t; (void)n; }
static inline void lwkt_gettoken(struct lwkt_token *t)
{ (void)t; }
static inline void lwkt_reltoken(struct lwkt_token *t)
{ (void)t; }
static int vm_map_find(struct vm_map *, vm_object_t, vm_ooffset_t,
                      vm_offset_t *, vm_size_t, int, int, int, int);
static vm_object_t vm_object_allocate(int, int);
static void vm_object_deallocate(vm_object_t);
#include <string.h>
static inline void *kmalloc(size_t sz, int type, int flags)
{ (void)type; (void)flags; return calloc(1, sz); }
static inline void kfree(void *p, int type) { (void)type; free(p); }
static inline unsigned long atop(size_t n) { return n / 4096; }
#define VM_PROT_READ  0x1
#define VM_PROT_WRITE 0x2
#define VM_PROT_ALL   (VM_PROT_READ|VM_PROT_WRITE)
#define FALSE         0
#define OBJT_DEFAULT  1
#define M_IPC         0
#define M_WAITOK      0
#define M_ZERO        0
#define E2BIG         7
#define ENOMEM        12
#define memory_barrier() do { } while (0)
static inline void wakeup(void *chan) { (void)chan; }
#else
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/token.h>
#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/vm_coherence.h>
#include <vm/vm_semantic.h>
#endif

struct zero_copy_channel {
        vm_object_t       zc_object;      /* backing object */
        vm_offset_t       zc_base;        /* mapped address */
        vm_size_t         zc_size;        /* region size */
        uint32_t          zc_semantic_type;/* semantic hint */
        uint32_t          zc_flags;
        uint64_t          zc_generation;  /* version */
        struct lwkt_token zc_token;       /* synchronization */
        struct coherence_state *zc_coh;   /* coherence tracking */
};

#define ZC_CROSS_VIRT      0x01
#define ZC_IMMUTABLE       0x02
#define ZC_NOTIFY          0x04
#define ZC_MAGIC   0x5a5a5a5a

struct zero_copy_header {
        uint32_t zch_magic;
        uint32_t zch_semantic_type;
        uint32_t zch_len;
        uint64_t zch_generation;
};

static int
map_zero_copy_region(struct proc *p, struct zero_copy_channel *chan, int prot)
{
        return vm_map_find(&p->p_vmspace->vm_map, chan->zc_object, 0,
                           &chan->zc_base, chan->zc_size, FALSE, prot,
                           VM_PROT_ALL, 0);
}

int
create_zero_copy_channel(struct proc *p1, struct proc *p2, size_t size,
                         int semantic_type,
                         struct zero_copy_channel **channelp)
{
        struct zero_copy_channel *chan;
        vm_object_t               obj;
        int                        error;

        chan = kmalloc(sizeof(*chan), M_IPC, M_WAITOK | M_ZERO);
        obj  = vm_object_allocate(OBJT_DEFAULT, atop(size));
        if (obj == NULL) {
                kfree(chan, M_IPC);
                return ENOMEM;
        }

        chan->zc_object        = obj;
        chan->zc_size          = size;
        chan->zc_semantic_type = semantic_type;
        lwkt_token_init(&chan->zc_token, "zc_channel");

        struct semantic_descriptor sem = {
            .domain = semantic_type,
            .attributes = 0,
            .version = 1
        };
        vm_coherence_establish(0, size, &sem, &chan->zc_coh);

        error = map_zero_copy_region(p1, chan, VM_PROT_READ | VM_PROT_WRITE);
        if (error == 0)
                error = map_zero_copy_region(p2, chan, VM_PROT_READ);
        if (error == 0) {
                vm_coherence_map(chan->zc_coh, 0, chan->zc_base,
                                 VM_PROT_READ | VM_PROT_WRITE);
                vm_coherence_map(chan->zc_coh, 1, chan->zc_base,
                                 VM_PROT_READ);
        }
        if (error) {
                vm_object_deallocate(obj);
                kfree(chan, M_IPC);
                return error;
        }
        *channelp = chan;
        return 0;
}

int
zero_copy_send(struct zero_copy_channel *chan, void *data, size_t len, int flags)
{
        struct zero_copy_header *hdr;
        if (len > chan->zc_size - sizeof(*hdr))
                return E2BIG;

        lwkt_gettoken(&chan->zc_token);
        hdr                      = (void *)chan->zc_base;
        hdr->zch_magic           = ZC_MAGIC;
        hdr->zch_semantic_type   = chan->zc_semantic_type;
        hdr->zch_len             = len;
        hdr->zch_generation      = ++chan->zc_generation;
        memory_barrier();
        vm_coherence_mark_write(chan->zc_coh, 0);
        if (flags & ZC_NOTIFY)
                wakeup(chan);
        lwkt_reltoken(&chan->zc_token);
        return 0;
}

#ifdef ZEROCOPY_STANDALONE
static int vm_map_find(struct vm_map *map, vm_object_t obj, vm_ooffset_t off,
                      vm_offset_t *addr, vm_size_t size, int align,
                      int prot, int maxprot, int flags)
{
        (void)map; (void)obj; (void)off; (void)align;
        (void)prot; (void)maxprot; (void)flags;
        *addr = (vm_offset_t)malloc(size);
        return *addr ? 0 : ENOMEM;
}

static vm_object_t vm_object_allocate(int type, int size)
{
        (void)type; (void)size; return (void *)1;
}

static void vm_object_deallocate(vm_object_t obj)
{
        (void)obj;
}

#endif
