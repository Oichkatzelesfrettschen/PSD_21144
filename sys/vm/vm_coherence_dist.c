#include <string.h>
#include <stdlib.h>

#include "include/vm_coherence_dist.h"

/* Initialize vector clock to zeros */
void
vc_init(struct vector_clock *vc)
{
    memset(vc->vals, 0, sizeof(vc->vals));
}

/* Increment local component in vector clock */
void
vc_increment(struct vector_clock *vc, int node)
{
    if (node >= 0 && node < MAX_DIST_NODES)
        vc->vals[node]++;
}

/* Merge two vector clocks by taking component-wise maximum */
void
vc_update(struct vector_clock *vc, const struct vector_clock *src)
{
    for (int i = 0; i < MAX_DIST_NODES; i++) {
        if (vc->vals[i] < src->vals[i])
            vc->vals[i] = src->vals[i];
    }
}

/* Return 1 if a happens-before b */
int
vc_happens_before(const struct vector_clock *a,
                  const struct vector_clock *b)
{
    int less = 0;

    for (int i = 0; i < MAX_DIST_NODES; i++) {
        if (a->vals[i] > b->vals[i])
            return 0;
        if (a->vals[i] < b->vals[i])
            less = 1;
    }
    return less;
}

/* Upgrade a coherence_state to distributed state */
int
vm_coherence_upgrade_distributed(struct coherence_state *local,
                                 uint8_t node_id,
                                 struct dist_coherence_state **out)
{
    struct dist_coherence_state *d;

    d = calloc(1, sizeof(*d));
    if (d == NULL)
        return ENOMEM;

    memcpy(&d->base, local, sizeof(*local));
    vc_init(&d->vc);
    d->node_id = node_id;
    *out = d;
    return 0;
}

/* Prepare export information and increment local clock */
int
vm_coherence_export(struct dist_coherence_state *dist,
                    struct network_export_info *info)
{
    vc_increment(&dist->vc, dist->node_id);

    if (info != NULL) {
        info->node_id = dist->node_id;
        info->domain = dist->base.semantic.domain;
        info->size = dist->base.size;
    }
    return 0;
}
