#include <assert.h>
#include <stddef.h>
#include <stdio.h>

struct proc {
	int dummy;
};
void setrunnable(struct proc* p) {
	(void) p;
}

void sched_preempt_init(void);
void sched_preempt(struct proc*); /* prototype from scheduler */

int main(void) {
        struct proc p = { 0 };
        sched_preempt_init();
        sched_preempt(&p);
        printf("sched_preempt ok\n");
        return 0;
}
