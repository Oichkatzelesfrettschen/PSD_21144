#ifndef _VM_COHERENCE_DIST_H_
#define _VM_COHERENCE_DIST_H_

#include <stdint.h>
#include "vm_coherence.h"

#define MAX_DIST_NODES 8

/* Vector clock used for causal consistency */
struct vector_clock {
    uint32_t vals[MAX_DIST_NODES];
};

/* Distributed coherence state wrapping a local coherence_state */
struct dist_coherence_state {
    struct coherence_state base; /* local coherence info */
    struct vector_clock   vc;    /* vector clock */
    uint8_t               node_id; /* ID of this node */
};

struct network_export_info {
    uint8_t                node_id;
    enum semantic_domain   domain;
    uint64_t               size;
};

void vc_init(struct vector_clock *vc);
void vc_increment(struct vector_clock *vc, int node);
void vc_update(struct vector_clock *vc, const struct vector_clock *src);
int  vc_happens_before(const struct vector_clock *a,
                       const struct vector_clock *b);

int vm_coherence_upgrade_distributed(struct coherence_state *local,
                                     uint8_t node_id,
                                     struct dist_coherence_state **out);
int vm_coherence_export(struct dist_coherence_state *dist,
                        struct network_export_info *info);

#endif /* _VM_COHERENCE_DIST_H_ */
