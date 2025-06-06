/*	$NetBSD: extern.h,v 1.4 2003/10/13 07:41:22 agc Exp $	*/

/*-
 * Copyright (c) 1991 Keith Muller.
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Keith Muller of the University of California, San Diego.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      from: @(#)extern.h	8.1 (Berkeley) 6/6/93
 *	$NetBSD: extern.h,v 1.4 2003/10/13 07:41:22 agc Exp $
 */

extern int eoptind;
extern char *eoptarg;

void	 addnum(char *, int, int);
int	 egetopt(int, char * const *, const char *);
void	 flsh_errs(void);
int	 horzcol(int, char **);
int	 inln(FILE *, char *, int, int *, int, int *);
int	 inskip(FILE *, int, int);
void	 mfail(void);
int	 mulfile(int, char **);
FILE	*nxtfile(int, char **, char **, char *, int);
int	 onecol(int, char **);
int	 otln(char *, int, int *, int *, int);
void	 pfail(void);
int	 prhead(char *, char *, int);
int	 prtail(int, int);
int	 setup(int, char **);
void	 terminate(int);
void	 usage(void);
int	 vertcol(int, char **);
