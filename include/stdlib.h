/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)stdlib.h	8.3.2 (2.11BSD) 1996/1/12
 *
 * Adapted from the 4.4-Lite CD.  The odds of a ANSI C compiler for 2.11BSD
 * being slipped under the door are not distinguishable from 0 - so the 
 * prototypes and ANSI ifdefs have been removed from this file. 
 *
 * Some functions (strtoul for example) do not exist yet but are retained in
 * this file because additions to libc.a are anticipated shortly.
 */

#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <sys/cdefs.h>

#if defined(__BSD_VISIBLE)
#include <sys/types.h>		/* for quad_t, etc. */
#endif

#include <machine/ansi.h>

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

#ifdef	_BSD_WCHAR_T_
#ifndef _ANSI_SOURCE
typedef	_BSD_RUNE_T_	rune_t;
#endif
typedef	_BSD_WCHAR_T_	wchar_t;
#undef	_BSD_WCHAR_T_
#endif

typedef struct {
	int quot;		/* quotient */
	int rem;		/* remainder */
} div_t;

typedef struct {
	long quot;		/* quotient */
	long rem;		/* remainder */
} ldiv_t;

#if !defined(_ANSI_SOURCE) && \
    (defined(_ISOC99_SOURCE) || (__STDC_VERSION__ - 0) >= 199901L || \
     (__cplusplus - 0) >= 201103L || defined(__BSD_VISIBLE))
typedef struct {
	/* LONGLONG */
	long long int quot;	/* quotient */
	/* LONGLONG */
	long long int rem;	/* remainder */
} lldiv_t;
#endif
#if defined(__BSD_VISIBLE)
typedef struct {
	quad_t 	quot;		/* quotient */
	quad_t 	rem;		/* remainder */
} qdiv_t;
#endif

#include <sys/null.h>

#define	EXIT_FAILURE	1
#define	EXIT_SUCCESS	0

#if defined(pdp11)
#define	RAND_MAX		0x7fff
#else
#define	RAND_MAX		0x7fffffff
#endif

extern size_t 		__mb_cur_max;
#define	MB_CUR_MAX	__mb_cur_max

__BEGIN_DECLS
__dead void 	_Exit(int) __attribute__((__noreturn__));
void 	abort(void) __attribute__((__noreturn__));
int 	abs(int);
int	atexit(void (*)(void));
double	atof(const char *);
int	atoi(const char *);
long	atol(const char *);
#ifndef __BSEARCH_DECLARED
#define __BSEARCH_DECLARED
/* also in search.h */
void	*bsearch(const void *, const void *, size_t, size_t, int (*)(const void *, const void *));
#endif /* __BSEARCH_DECLARED */
void	*calloc(size_t, size_t);
div_t	div(int, int);
__dead void 	exit(int) __attribute__((__noreturn__));
void	free(void *);
__aconst char 	*getenv(const char *);
long 	labs(long);
ldiv_t	ldiv(long, long);
void	*malloc(size_t);
void	freezero(void *, size_t);
void	qsort(void *, size_t, size_t, int (*)(const void *, const void *));
int	rand(void);
void	*realloc(void *, size_t);
void	*reallocarray(void *, size_t, size_t);
void	*recallocarray(void *, size_t, size_t, size_t);
void	srand(unsigned);
double	strtod(const char * __restrict, char ** __restrict);
long	strtol(const char * __restrict, char ** __restrict, int);
unsigned long strtoul(const char * __restrict, char ** __restrict, int);
int	 system(const char *);

/* Floating output conversion */
char	*ecvt(double, int, int *, int *);
char	*fcvt(double, int, int *, int *);
char	*gcvt(double, int, char *);

/* convert longs to 3-byte disk addresses */
void	ltol3(char *, long *, int);
/* convert 3-byte disk addresses to longs */
void 	l3tol(long *, char *, int);

int	mblen(const char *, size_t);
size_t	mbstowcs(wchar_t * __restrict, const char * __restrict, size_t);
int	wctomb(char *, wchar_t);
int	mbtowc(wchar_t * __restrict, const char * __restrict, size_t);
size_t	wcstombs(char * __restrict, const wchar_t * __restrict, size_t);

#if defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE) || defined(__BSD_VISIBLE)

/*
 * IEEE Std 1003.1c-95, also adopted by X/Open CAE Spec Issue 5 Version 2
 */
#if (_POSIX_C_SOURCE - 0) >= 199506L || (_XOPEN_SOURCE - 0) >= 500 || defined(_REENTRANT) || defined(__BSD_VISIBLE)
int	 rand_r(unsigned int *);
#endif

/*
 * X/Open Portability Guide >= Issue 4
 */
#if (_XOPEN_SOURCE - 0) >= 4 || defined(__BSD_VISIBLE)
double	 drand48(void);
double	 erand48(unsigned short[3]);
long	 jrand48(unsigned short[3]);
void	 lcong48(unsigned short[7]);
long	 lrand48(void);
long	 mrand48(void);
long	 nrand48(unsigned short[3]);
unsigned short *seed48(unsigned short[3]);
void	 srand48(long);

int	 putenv(const char *);

#endif

/*
 * X/Open Portability Guide >= Issue 4 Version 2
 */
#if (defined(_XOPEN_SOURCE) && defined(_XOPEN_SOURCE_EXTENDED)) || (_XOPEN_SOURCE - 0) >= 500 || defined(__BSD_VISIBLE)
long	 a64l(const char *);
char	 *l64a(long);

char	 *initstate(unsigned long, char *, size_t);
long	 random(void);
char	 *setstate(char *);
void	 srandom(unsigned long);

char	 *mkdtemp(char *);
int	 mkstemp(char *);
#ifdef __MKTEMP_OK__
	__RENAME(_mktemp)
#endif
	;
int	 setkey(const char *);
char	 *realpath(const char *, char *);
int	 ttyslot(void);
void	 *valloc(size_t);		/* obsoleted by malloc() */
int	 getsubopt(char **, char * const *, char **);
#endif

/*
 * ISO C99
 */
#if defined(_ISOC99_SOURCE) || (__STDC_VERSION__ - 0) >= 199901L || \
    defined(__BSD_VISIBLE) || (__cplusplus - 0) >= 201103L
/* LONGLONG */
long long int	atoll(const char *);
/* LONGLONG */
long long int	llabs(long long int);
/* LONGLONG */
lldiv_t		lldiv(long long int, long long int);
/* LONGLONG */
long long int	strtoll(const char * __restrict, char ** __restrict, int);
/* LONGLONG */
unsigned long long int
		strtoull(const char * __restrict, char ** __restrict, int);
float		strtof(const char * __restrict, char ** __restrict);
long double	strtold(const char * __restrict, char ** __restrict);
#endif

#if defined(_ISOC11_SOURCE) || (__STDC_VERSION__ - 0) >= 201101L || \
    defined(__BSD_VISIBLE) || (__cplusplus - 0) >= 201103L
void	*aligned_alloc(size_t, size_t);
int		at_quick_exit(void (*)(void));
__dead void quick_exit(int);
#endif

/*
 * The Open Group Base Specifications, Issue 6; IEEE Std 1003.1-2001 (POSIX)
 */
#if (_POSIX_C_SOURCE - 0) >= 200112L || (_XOPEN_SOURCE - 0) >= 600 || defined(__BSD_VISIBLE)
int	 setenv(const char *, const char *, int);
int	 unsetenv(const char *);

int	 posix_memalign(void **, size_t, size_t);
#endif

/*
 * Implementation-defined extensions
 */
#if defined(__BSD_VISIBLE)
#if defined(alloca) && (alloca == __builtin_alloca) && (__GNUC__ < 2)
void	*alloca(int);     /* built-in for gcc */
#else
void	*alloca(size_t);
#endif /* __GNUC__ */

char	 *getbsize(int *, long *);
char	 *cgetcap(char *, const char *, int);
int	 cgetclose(void);
int	 cgetent(char **, char **, const char *);
int	 cgetfirst(char **, char **);
int	 cgetmatch(const char *, const char *);
int	 cgetnext(char **, char **);
int	 cgetnum(char *, const char *, long *);
int	 cgetset(const char *);
int	 cgetstr(char *, const char *, char **);
int	 cgetustr(char *, const char *, char **);
void     csetexpandtc(int);

int	 daemon(int, int);
char 	 *devname(dev_t, mode_t);
int	 getloadavg(double [], int);

ssize_t	 hmac(const char *, const void *, size_t, const void *, size_t, void *, size_t);

void	cfree(void *);

int	heapsort(void *, size_t, size_t, int (*)(const void *, const void *));
int	mergesort (void *, size_t, size_t, int (*)(const void *, const void *));
int	radixsort(const unsigned char **, int, const unsigned char *, unsigned);
int	sradixsort(const unsigned char **, int, const unsigned char *, unsigned);

void	 mi_vector_hash(const void * __restrict, size_t, uint32_t, uint32_t[3]);

void	 setproctitle(const char *, ...) __attribute__((__format__(__printf__, 1, 2)));
const char *getprogname(void) __attribute__((__const__));
void	setprogname(const char *);

quad_t	 qabs(quad_t);
qdiv_t	 qdiv(quad_t, quad_t);
quad_t	 strtoq(const char * __restrict, char ** __restrict, int);
u_quad_t strtouq(const char * __restrict, char ** __restrict, int);

	/* LONGLONG */
long long strsuftoll(const char *, const char *, long long, long long);
	/* LONGLONG */
long long strsuftollx(const char *, const char *, long long, long long, char *, size_t);

int	 l64a_r(long, char *, int);

size_t	 shquote(const char *, char *, size_t);
//size_t shquotev(int, char * const *, char *, size_t);

int	 reallocarr(void *, size_t, size_t);

u_int32_t arc4random(void);
u_int32_t arc4random_uniform(u_int32_t);
void      arc4random_buf(void *, size_t);
void	  arc4random_stir(void);
void	  arc4random_addrandom(unsigned char *, int);

#endif /* __BSD_VISIBLE */
#endif /* _POSIX_C_SOURCE || _XOPEN_SOURCE || __BSD_VISIBLE */

#if (_POSIX_C_SOURCE - 0) >= 200809L || defined(__BSD_VISIBLE)
#  ifndef __LOCALE_T_DECLARED
typedef struct _locale *locale_t;
#  define __LOCALE_T_DECLARED
#  endif

#if defined(__BSD_VISIBLE)
int	mblen_l(const char *, size_t, locale_t);
size_t	mbstowcs_l(wchar_t * __restrict, const char * __restrict, size_t, locale_t);
int	wctomb_l(char *, wchar_t, locale_t);
int	mbtowc_l(wchar_t * __restrict, const char * __restrict, size_t, locale_t);
size_t	wcstombs_l(char * __restrict, const wchar_t * __restrict, size_t, locale_t);
#endif /* __BSD_VISIBLE */
#endif /* _POSIX_C_SOURCE >= 200809 || __BSD_VISIBLE */
__END_DECLS
#endif /* _STDLIB_H_ */
