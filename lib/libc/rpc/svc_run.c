/*	$NetBSD: svc_run.c,v 1.13 1999/01/20 11:37:39 lukem Exp $	*/

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char *sccsid = "@(#)svc_run.c 1.1 87/10/13 Copyr 1984 Sun Micro";
static char *sccsid = "@(#)svc_run.c	2.1 88/07/29 4.0 RPCSRC";
#else
__RCSID("$NetBSD: svc_run.c,v 1.13 1999/01/20 11:37:39 lukem Exp $");
#endif
#endif

/*
 * This is the rpc server side idle loop
 * Wait for input, call server program.
 */
#include "namespace.h"

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rpc/rpc.h>

#ifdef __weak_alias
__weak_alias(svc_run,_svc_run);
#endif

#ifdef __SELECT_DECLARED
static int svc_run_select(fd_set *);
#else
static int svc_run_poll(struct pollfd *);
#endif

void
svc_run(void)
{
	fd_set readfds;
	struct pollfd readpfd;
	int ret;

	for (;;) {
		readpfd = svc_pollfd;
		readfds = svc_fdset;

#ifdef __SELECT_DECLARED
		ret = svc_run_select(&readfds);
#else
		ret = svc_run_poll(&readpfd);
#endif
		switch (ret) {
		case -1:
			if (errno == EINTR) {
				continue;
			}
			return;
		case 0:
			continue;
		default:
			svc_getreqset_mix(&readpfd, &readfds);
		}
	}
}

#ifdef __SELECT_DECLARED
static int
svc_run_select(readfds)
	fd_set *readfds;
{
	int ret;

	ret = select(svc_maxfd+1, readfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0);
	if (ret != 0) {
		warn("svc_run: - select failed");
		return (-1);
	}
	return (0);
}

#else

static int
svc_run_poll(readpfd)
	struct pollfd *readpfd;
{
	struct pollfd *newp;
	int ret, saved_max_pollfd = 0;

	if (svc_max_pollfd > saved_max_pollfd) {
		newp = reallocarray(readpfd, svc_max_pollfd, sizeof(*readpfd));
		if (newp == NULL) {
			return (-1);			/* XXX */
		}
		saved_max_pollfd = svc_max_pollfd;
		readpfd = newp;
	}
	memcpy(readpfd, &svc_pollfd, sizeof(*readpfd) * svc_max_pollfd);
	ret = poll(readpfd, svc_max_pollfd, INFTIM);
	if (ret != 0) {
		warn("svc_run: - poll failed");
		return (-1);
	}
	return (0);
}
#endif
