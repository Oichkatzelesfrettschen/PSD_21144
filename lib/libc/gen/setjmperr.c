/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)setjmperr.c	5.2 (Berkeley) 3/9/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <setjmp.h>			/* include any machine defines */
#include <unistd.h>

#define ERRMSG	"longjmp botch.\n"

/*
 * This routine is called from longjmp() when an error occurs.
 * Programs that wish to exit gracefully from this error may
 * write their own versions.
 * If this routine returns, the program is aborted.
 */
void
longjmperror(void)
{
	(void)write(STDERR_FILENO, ERRMSG, sizeof(ERRMSG)-1);
}
