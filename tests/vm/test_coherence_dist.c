#include <assert.h>
#include <stdio.h>
#include <string.h>

#define COHERENCE_STANDALONE
#include "../../sys/vm/include/vm_coherence.h"
#include "../../sys/vm/include/vm_coherence_dist.h"

int main(void)
{
    struct semantic_descriptor desc = { SEM_MESSAGE, 0, 1 };
    struct coherence_state *local;
    struct dist_coherence_state *dist;
    struct network_export_info info;

    vm_coherence_init();
    assert(vm_coherence_establish(0, 4096, &desc, &local) == 0);
    assert(vm_coherence_upgrade_distributed(local, 1, &dist) == 0);

    vc_increment(&dist->vc, 1);
    vc_increment(&dist->vc, 2);

    struct vector_clock other;
    vc_init(&other);
    other.vals[1] = 2;
    other.vals[2] = 1;

    assert(vc_happens_before(&dist->vc, &other));

    vc_update(&dist->vc, &other);

    assert(dist->vc.vals[1] == 2);
    assert(dist->vc.vals[2] == 1);
    assert(!vc_happens_before(&other, &dist->vc));

    vm_coherence_export(dist, &info);
    assert(info.node_id == 1);
    assert(info.size == 4096);

    printf("coherence_dist ok\n");
    return 0;
}
