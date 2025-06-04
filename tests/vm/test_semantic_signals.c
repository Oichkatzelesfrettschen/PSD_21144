#include <assert.h>
#include <signal.h>
#include <stdio.h>

#define SIG_UNIFIED_STANDALONE 1
#define NUM_SEMANTIC_DOMAINS 4

#include "../../sys/kern/kern_sig_unified.c"

static int delivered;
static int delivered_domain;

static void
handler(int sig)
{
        (void)sig;
        delivered = 1;
        delivered_domain = 2; /* stub domain set in semantic_sendsig */
}

int
main(void)
{
        kern_sig_unified_init();
        semantic_sendsig(handler, SIGUSR1, 0, 0, 2, 0);
        assert(delivered == 1);
        assert(delivered_domain == 2);
        printf("semantic_signals ok\n");
        return 0;
}
