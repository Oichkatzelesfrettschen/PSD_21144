/*	$NetBSD: getaddrinfo.c,v 1.91.6.1 2009/01/26 00:27:34 snj Exp $	*/
/*	$KAME: getaddrinfo.c,v 1.29 2000/08/31 17:26:57 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Issues to be discussed:
 * - Return values.  There are nonstandard return values defined and used
 *   in the source code.  This is because RFC2553 is silent about which error
 *   code must be returned for which situation.
 * - IPv4 classful (shortened) form.  RFC2553 is silent about it.  XNET 5.2
 *   says to use inet_aton() to convert IPv4 numeric to binary (alows
 *   classful form as a result).
 *   current code - disallow classful form for IPv4 (due to use of inet_pton).
 * - freeaddrinfo(NULL).  RFC2553 is silent about it.  XNET 5.2 says it is
 *   invalid.
 *   current code - SEGV on freeaddrinfo(NULL)
 * Note:
 * - The code filters out AFs that are not supported by the kernel,
 *   when globbing NULL hostname (to loopback, or wildcard).  Is it the right
 *   thing to do?  What is the relationship with post-RFC2553 AI_ADDRCONFIG
 *   in ai_flags?
 * - (post-2553) semantics of AI_ADDRCONFIG itself is too vague.
 *   (1) what should we do against numeric hostname (2) what should we do
 *   against NULL hostname (3) what is AI_ADDRCONFIG itself.  AF not ready?
 *   non-loopback address configured?  global address configured?
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: getaddrinfo.c,v 1.91.6.1 2009/01/26 00:27:34 snj Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <resolv.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <syslog.h>
#include <stdarg.h>
#include <nsswitch.h>

#ifdef YP
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#endif

#include "resolv/res_private.h"
#include "servent.h"

#ifdef __weak_alias
__weak_alias(getaddrinfo,_getaddrinfo)
__weak_alias(freeaddrinfo,_freeaddrinfo)
__weak_alias(gai_strerror,_gai_strerror)
#endif

#define SUCCESS 0
#define ANY 0
#define YES 1
#define NO  0

static const char in_addrany[] = { 0, 0, 0, 0 };
static const char in_loopback[] = { 127, 0, 0, 1 };
#ifdef INET6
static const char in6_addrany[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static const char in6_loopback[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
};
#endif

static const struct afd {
	int a_af;
	int a_addrlen;
	int a_socklen;
	int a_off;
	const char *a_addrany;
	const char *a_loopback;	
	int a_scoped;
} afdl [] = {
#ifdef INET6
		{
				.a_af = PF_INET6,
				.a_addrlen = sizeof(struct in6_addr),
				.a_socklen = sizeof(struct sockaddr_in6),
				.a_off = offsetof(struct sockaddr_in6, sin6_addr),
				.a_addrany = in6_addrany,
				.a_loopback = in6_loopback,
				.a_scoped = 1,
		},
#endif
		{
				.a_af = PF_INET,
				.a_addrlen = sizeof(struct in_addr),
				.a_socklen = sizeof(struct sockaddr_in),
				.a_off = offsetof(struct sockaddr_in, sin_addr),
				.a_addrany = in_addrany,
				.a_loopback = in_loopback,
				.a_scoped = 1,
		},
		{
				.a_af = 0,
				.a_addrlen = 0,
				.a_socklen = 0,
				.a_off = 0,
				.a_addrany = NULL,
				.a_loopback = NULL,
				.a_scoped = 0,
		},
};

struct explore {
	int e_af;
	int e_socktype;
	int e_protocol;
	const char *e_protostr;
	int e_wild;
#define WILD_AF(ex)			((ex)->e_wild & 0x01)
#define WILD_SOCKTYPE(ex)	((ex)->e_wild & 0x02)
#define WILD_PROTOCOL(ex)	((ex)->e_wild & 0x04)
};

static const struct explore explore[] = {
#if 0
	{ PF_LOCAL, 0, ANY, ANY, NULL, 0x01 },
#endif
#ifdef INET6
	{ PF_INET6, SOCK_DGRAM, IPPROTO_UDP, "udp", 0x07 },
	{ PF_INET6, SOCK_STREAM, IPPROTO_TCP, "tcp", 0x07 },
	{ PF_INET6, SOCK_RAW, ANY, NULL, 0x05 },
#endif
	{ PF_INET, SOCK_DGRAM, IPPROTO_UDP, "udp", 0x07 },
	{ PF_INET, SOCK_STREAM, IPPROTO_TCP, "tcp", 0x07 },
	{ PF_INET, SOCK_RAW, ANY, NULL, 0x05 },
	{ PF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, "udp", 0x07 },
	{ PF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, "tcp", 0x07 },
	{ PF_UNSPEC, SOCK_RAW, ANY, NULL, 0x05 },
	{ -1, 0, 0, NULL, 0 },
};

#ifdef INET6
#define PTON_MAX	16
#else
#define PTON_MAX	4
#endif

static const ns_src default_dns_files[] = {
	{ NSSRC_FILES, 	NS_SUCCESS },
	{ NSSRC_DNS, 	NS_SUCCESS },
	{ 0, 0 }
};

#define MAXPACKET	(64*1024)

typedef union {
	HEADER hdr;
	u_char buf[MAXPACKET];
} querybuf;

struct res_target {
	struct res_target *next;
	const char *name;	/* domain name */
	int qclass, qtype;	/* class and type of query */
	u_char *answer;		/* buffer to put answer */
	int anslen;		/* size of answer buffer */
	int n;			/* result length */
};

static int str2number(const char *);
static int explore_fqdn(const struct addrinfo *, const char *,
	const char *, struct addrinfo **, struct servent_data *);
static int explore_null(const struct addrinfo *,
	const char *, struct addrinfo **, struct servent_data *);
static int explore_numeric(const struct addrinfo *, const char *,
	const char *, struct addrinfo **, const char *, struct servent_data *);
static int explore_numeric_scope(const struct addrinfo *, const char *,
	const char *, struct addrinfo **, struct servent_data *);
static int get_canonname(const struct addrinfo *,
	struct addrinfo *, const char *);
static struct addrinfo *get_ai(const struct addrinfo *,
	const struct afd *, const char *);
static int get_portmatch(const struct addrinfo *, const char *,
    struct servent_data *);
static int get_port(const struct addrinfo *, const char *, int,
    struct servent_data *);
static const struct afd *find_afd(int);
#ifdef INET6
static int ip6_str2scopeid(char *, struct sockaddr_in6 *, u_int32_t *);
#endif

static struct addrinfo *getanswer(const querybuf *, int, const char *, int,
	const struct addrinfo *);
static void aisort(struct addrinfo *s, res_state res);
static int _dns_getaddrinfo(void *, void *, va_list);
static void _sethtent(FILE **);
static void _endhtent(FILE **);
static struct addrinfo *_gethtent(FILE **, const char *,
    const struct addrinfo *);
static int _files_getaddrinfo(void *, void *, va_list);
#ifdef YP
static struct addrinfo *_yphostent(char *, const struct addrinfo *);
static int _yp_getaddrinfo(void *, void *, va_list);
#endif

static int res_queryN(const char *, struct res_target *, res_state);
static int res_searchN(const char *, struct res_target *, res_state);
static int res_querydomainN(const char *, const char *,
	struct res_target *, res_state);

static const char * const ai_errlist[] = {
	"Success",
	"Address family for hostname not supported",	/* EAI_ADDRFAMILY */
	"Temporary failure in name resolution",		/* EAI_AGAIN      */
	"Invalid value for ai_flags",		       	/* EAI_BADFLAGS   */
	"Non-recoverable failure in name resolution", 	/* EAI_FAIL       */
	"ai_family not supported",			/* EAI_FAMILY     */
	"Memory allocation failure", 			/* EAI_MEMORY     */
	"No address associated with hostname", 		/* EAI_NODATA     */
	"hostname nor servname provided, or not known",	/* EAI_NONAME     */
	"servname not supported for ai_socktype",	/* EAI_SERVICE    */
	"ai_socktype not supported", 			/* EAI_SOCKTYPE   */
	"System error returned in errno", 		/* EAI_SYSTEM     */
	"Invalid value for hints",			/* EAI_BADHINTS	  */
	"Resolved protocol is unknown",			/* EAI_PROTOCOL   */
	"Argument buffer overflow",			/* EAI_OVERFLOW   */
	"Unknown error", 				/* EAI_MAX        */
};

/* XXX macros that make external reference is BAD. */

#define GET_AI(ai, afd, addr) 					\
do { 								\
	/* external reference: pai, error, and label free */ 	\
	(ai) = get_ai(pai, (afd), (addr)); 			\
	if ((ai) == NULL) { 					\
		error = EAI_MEMORY; 				\
		goto free; 					\
	} 							\
} while (/*CONSTCOND*/0)

#define GET_PORT(ai, serv, svd) 				\
do { 								\
	/* external reference: error and label free */ 		\
	error = get_port((ai), (serv), 0, (svd)); 		\
	if (error != 0) 					\
		goto free; 					\
} while (/*CONSTCOND*/0)

#define GET_CANONNAME(ai, str) 					\
do { 								\
	/* external reference: pai, error and label free */ 	\
	error = get_canonname(pai, (ai), (str)); 		\
	if (error != 0) 					\
		goto free; 					\
} while (/*CONSTCOND*/0)

#define ERR(err) 						\
do { 								\
	/* external reference: error, and label bad */ 		\
	error = (err); 						\
	goto bad; 						\
	/*NOTREACHED*/ 						\
} while (/*CONSTCOND*/0)

#define MATCH_FAMILY(x, y, w) 						\
	((x) == (y) || (/*CONSTCOND*/(w) && ((x) == PF_UNSPEC || 	\
	    (y) == PF_UNSPEC)))	
#define MATCH(x, y, w) 							\
	((x) == (y) || (/*CONSTCOND*/(w) && ((x) == ANY || (y) == ANY)))

const char *
gai_strerror(int ecode)
{
	if (ecode < 0 || ecode > EAI_MAX)
		ecode = EAI_MAX;
	return ai_errlist[ecode];
}

void
freeaddrinfo(struct addrinfo *ai)
{
	struct addrinfo *next;

	_DIAGASSERT(ai != NULL);

	do {
		next = ai->ai_next;
		if (ai->ai_canonname)
			free(ai->ai_canonname);
		/* no need to free(ai->ai_addr) */
		free(ai);
		ai = next;
	} while (ai);
}

static int
str2number(const char *p)
{
	char *ep;
	unsigned long v;

	_DIAGASSERT(p != NULL);

	if (*p == '\0')
		return -1;
	ep = NULL;
	errno = 0;
	v = strtoul(p, &ep, 10);
	if (errno == 0 && ep && *ep == '\0' && v <= UINT_MAX)
		return v;
	else
		return -1;
}

int
getaddrinfo(const char *hostname, const char *servname,
    const struct addrinfo *hints, struct addrinfo **res)
{
	struct addrinfo sentinel;
	struct addrinfo *cur;
	int error = 0;
	struct addrinfo ai;
	struct addrinfo ai0;
	struct addrinfo *pai;
	const struct explore *ex;
	struct servent_data svd;

	/* hostname is allowed to be NULL */
	/* servname is allowed to be NULL */
	/* hints is allowed to be NULL */
	_DIAGASSERT(res != NULL);

	(void)memset(&svd, 0, sizeof(svd));
	memset(&sentinel, 0, sizeof(sentinel));
	cur = &sentinel;
	memset(&ai, 0, sizeof(ai));
	pai = &ai;
	pai->ai_flags = 0;
	pai->ai_family = PF_UNSPEC;
	pai->ai_socktype = ANY;
	pai->ai_protocol = ANY;
	pai->ai_addrlen = 0;
	pai->ai_canonname = NULL;
	pai->ai_addr = NULL;
	pai->ai_next = NULL;
	
	if (hostname == NULL && servname == NULL)
		return EAI_NONAME;
	if (hints) {
		/* error check for hints */
		if (hints->ai_addrlen || hints->ai_canonname ||
		    hints->ai_addr || hints->ai_next)
			ERR(EAI_BADHINTS); /* xxx */
		if (hints->ai_flags & ~AI_MASK)
			ERR(EAI_BADFLAGS);
		switch (hints->ai_family) {
		case PF_UNSPEC:
		case PF_INET:
#ifdef INET6
		case PF_INET6:
#endif
			break;
		default:
			ERR(EAI_FAMILY);
		}
		memcpy(pai, hints, sizeof(*pai));

		/*
		 * if both socktype/protocol are specified, check if they
		 * are meaningful combination.
		 */
		if (pai->ai_socktype != ANY && pai->ai_protocol != ANY) {
			for (ex = explore; ex->e_af >= 0; ex++) {
				if (pai->ai_family != ex->e_af)
					continue;
				if (ex->e_socktype == ANY)
					continue;
				if (ex->e_protocol == ANY)
					continue;
				if (pai->ai_socktype == ex->e_socktype
				 && pai->ai_protocol != ex->e_protocol) {
					ERR(EAI_BADHINTS);
				}
			}
		}
	}

	/*
	 * check for special cases.  (1) numeric servname is disallowed if
	 * socktype/protocol are left unspecified. (2) servname is disallowed
	 * for raw and other inet{,6} sockets.
	 */
	if (MATCH_FAMILY(pai->ai_family, PF_INET, 1)
#ifdef PF_INET6
	 || MATCH_FAMILY(pai->ai_family, PF_INET6, 1)
#endif
	    ) {
		ai0 = *pai;	/* backup *pai */

		if (pai->ai_family == PF_UNSPEC) {
#ifdef PF_INET6
			pai->ai_family = PF_INET6;
#else
			pai->ai_family = PF_INET;
#endif
		}
		error = get_portmatch(pai, servname, &svd);
		if (error)
			ERR(error);

		*pai = ai0;
	}

	ai0 = *pai;

	/* NULL hostname, or numeric hostname */
	for (ex = explore; ex->e_af >= 0; ex++) {
		*pai = ai0;

		/* PF_UNSPEC entries are prepared for DNS queries only */
		if (ex->e_af == PF_UNSPEC)
			continue;

		if (!MATCH_FAMILY(pai->ai_family, ex->e_af, WILD_AF(ex)))
			continue;
		if (!MATCH(pai->ai_socktype, ex->e_socktype, WILD_SOCKTYPE(ex)))
			continue;
		if (!MATCH(pai->ai_protocol, ex->e_protocol, WILD_PROTOCOL(ex)))
			continue;

		if (pai->ai_family == PF_UNSPEC)
			pai->ai_family = ex->e_af;
		if (pai->ai_socktype == ANY && ex->e_socktype != ANY)
			pai->ai_socktype = ex->e_socktype;
		if (pai->ai_protocol == ANY && ex->e_protocol != ANY)
			pai->ai_protocol = ex->e_protocol;

		if (hostname == NULL)
			error = explore_null(pai, servname, &cur->ai_next,
			    &svd);
		else
			error = explore_numeric_scope(pai, hostname, servname,
			    &cur->ai_next, &svd);

		if (error)
			goto free;

		while (cur->ai_next)
			cur = cur->ai_next;
	}

	/*
	 * XXX
	 * If numeric representation of AF1 can be interpreted as FQDN
	 * representation of AF2, we need to think again about the code below.
	 */
	if (sentinel.ai_next)
		goto good;

	if (hostname == NULL)
		ERR(EAI_NODATA);
	if (pai->ai_flags & AI_NUMERICHOST)
		ERR(EAI_NONAME);

	/*
	 * hostname as alphabetical name.
	 * we would like to prefer AF_INET6 than AF_INET, so we'll make a
	 * outer loop by AFs.
	 */
	for (ex = explore; ex->e_af >= 0; ex++) {
		*pai = ai0;

		/* require exact match for family field */
		if (pai->ai_family != ex->e_af)
			continue;

		if (!MATCH(pai->ai_socktype, ex->e_socktype,
				WILD_SOCKTYPE(ex))) {
			continue;
		}
		if (!MATCH(pai->ai_protocol, ex->e_protocol,
				WILD_PROTOCOL(ex))) {
			continue;
		}

		if (pai->ai_socktype == ANY && ex->e_socktype != ANY)
			pai->ai_socktype = ex->e_socktype;
		if (pai->ai_protocol == ANY && ex->e_protocol != ANY)
			pai->ai_protocol = ex->e_protocol;

		error = explore_fqdn(pai, hostname, servname, &cur->ai_next,
		    &svd);

		while (cur && cur->ai_next)
			cur = cur->ai_next;
	}

	/* XXX */
	if (sentinel.ai_next)
		error = 0;

	if (error)
		goto free;

	if (sentinel.ai_next) {
 good:
		endservent_r(&svd);
		*res = sentinel.ai_next;
		return SUCCESS;
	} else
		error = EAI_FAIL;
 free:
 bad:
	endservent_r(&svd);
	if (sentinel.ai_next)
		freeaddrinfo(sentinel.ai_next);
	*res = NULL;
	return error;
}

/*
 * FQDN hostname, DNS lookup
 */
static int
explore_fqdn(const struct addrinfo *pai, const char *hostname,
    const char *servname, struct addrinfo **res, struct servent_data *svd)
{
	struct addrinfo *result;
	struct addrinfo *cur;
	int error = 0;
	static const ns_dtab dtab[] = {
		NS_FILES_CB(_files_getaddrinfo, NULL)
		{ NSSRC_DNS, _dns_getaddrinfo, NULL },	/* force -DHESIOD */
		NS_NIS_CB(_yp_getaddrinfo, NULL)
		NS_NULL_CB
	};

	_DIAGASSERT(pai != NULL);
	/* hostname may be NULL */
	/* servname may be NULL */
	_DIAGASSERT(res != NULL);

	result = NULL;

	/*
	 * if the servname does not match socktype/protocol, ignore it.
	 */
	if (get_portmatch(pai, servname, svd) != 0)
		return 0;

	switch (nsdispatch(&result, dtab, NSDB_HOSTS, "getaddrinfo",
			default_dns_files, hostname, pai)) {
	case NS_TRYAGAIN:
		error = EAI_AGAIN;
		goto free;
	case NS_UNAVAIL:
		error = EAI_FAIL;
		goto free;
	case NS_NOTFOUND:
		error = EAI_NODATA;
		goto free;
	case NS_SUCCESS:
		error = 0;
		for (cur = result; cur; cur = cur->ai_next) {
			GET_PORT(cur, servname, svd);
			/* canonname should be filled already */
		}
		break;
	}

	*res = result;

	return 0;

free:
	if (result)
		freeaddrinfo(result);
	return error;
}

/*
 * hostname == NULL.
 * passive socket -> anyaddr (0.0.0.0 or ::)
 * non-passive socket -> localhost (127.0.0.1 or ::1)
 */
static int
explore_null(const struct addrinfo *pai, const char *servname,
    struct addrinfo **res, struct servent_data *svd)
{
	int s;
	const struct afd *afd;
	struct addrinfo *cur;
	struct addrinfo sentinel;
	int error;

	_DIAGASSERT(pai != NULL);
	/* servname may be NULL */
	_DIAGASSERT(res != NULL);

	*res = NULL;
	sentinel.ai_next = NULL;
	cur = &sentinel;

	/*
	 * filter out AFs that are not supported by the kernel
	 * XXX errno?
	 */
	s = socket(pai->ai_family, SOCK_DGRAM, 0);
	if (s < 0) {
		if (errno != EMFILE)
			return 0;
	} else
		close(s);

	/*
	 * if the servname does not match socktype/protocol, ignore it.
	 */
	if (get_portmatch(pai, servname, svd) != 0)
		return 0;

	afd = find_afd(pai->ai_family);
	if (afd == NULL)
		return 0;

	if (pai->ai_flags & AI_PASSIVE) {
		GET_AI(cur->ai_next, afd, afd->a_addrany);
		/* xxx meaningless?
		 * GET_CANONNAME(cur->ai_next, "anyaddr");
		 */
		GET_PORT(cur->ai_next, servname, svd);
	} else {
		GET_AI(cur->ai_next, afd, afd->a_loopback);
		/* xxx meaningless?
		 * GET_CANONNAME(cur->ai_next, "localhost");
		 */
		GET_PORT(cur->ai_next, servname, svd);
	}
	cur = cur->ai_next;

	*res = sentinel.ai_next;
	return 0;

free:
	if (sentinel.ai_next)
		freeaddrinfo(sentinel.ai_next);
	return error;
}

/*
 * numeric hostname
 */
static int
explore_numeric(const struct addrinfo *pai, const char *hostname,
    const char *servname, struct addrinfo **res, const char *canonname,
    struct servent_data *svd)
{
	const struct afd *afd;
	struct addrinfo *cur;
	struct addrinfo sentinel;
	int error;
	char pton[PTON_MAX];

	_DIAGASSERT(pai != NULL);
	/* hostname may be NULL */
	/* servname may be NULL */
	_DIAGASSERT(res != NULL);

	*res = NULL;
	sentinel.ai_next = NULL;
	cur = &sentinel;

	/*
	 * if the servname does not match socktype/protocol, ignore it.
	 */
	if (get_portmatch(pai, servname, svd) != 0)
		return 0;

	afd = find_afd(pai->ai_family);
	if (afd == NULL)
		return 0;

	switch (afd->a_af) {
#if 0 /*X/Open spec*/
	case AF_INET:
		if (inet_aton(hostname, (struct in_addr *)pton) == 1) {
			if (pai->ai_family == afd->a_af ||
			    pai->ai_family == PF_UNSPEC /*?*/) {
				GET_AI(cur->ai_next, afd, pton);
				GET_PORT(cur->ai_next, servname, svd);
				if ((pai->ai_flags & AI_CANONNAME)) {
					/*
					 * Set the numeric address itself as
					 * the canonical name, based on a
					 * clarification in rfc2553bis-03.
					 */
					GET_CANONNAME(cur->ai_next, canonname);
				}
				while (cur && cur->ai_next)
					cur = cur->ai_next;
			} else
				ERR(EAI_FAMILY);	/*xxx*/
		}
		break;
#endif
	default:
		if (inet_pton(afd->a_af, hostname, pton) == 1) {
			if (pai->ai_family == afd->a_af ||
			    pai->ai_family == PF_UNSPEC /*?*/) {
				GET_AI(cur->ai_next, afd, pton);
				GET_PORT(cur->ai_next, servname, svd);
				if ((pai->ai_flags & AI_CANONNAME)) {
					/*
					 * Set the numeric address itself as
					 * the canonical name, based on a
					 * clarification in rfc2553bis-03.
					 */
					GET_CANONNAME(cur->ai_next, canonname);
				}
				while (cur->ai_next)
					cur = cur->ai_next;
			} else
				ERR(EAI_FAMILY);	/*xxx*/
		}
		break;
	}

	*res = sentinel.ai_next;
	return 0;

free:
bad:
	if (sentinel.ai_next)
		freeaddrinfo(sentinel.ai_next);
	return error;
}

/*
 * numeric hostname with scope
 */
static int
explore_numeric_scope(const struct addrinfo *pai, const char *hostname,
    const char *servname, struct addrinfo **res, struct servent_data *svd)
{
#if !defined(SCOPE_DELIMITER) || !defined(INET6)
	return explore_numeric(pai, hostname, servname, res, hostname, svd);
#else
	const struct afd *afd;
	struct addrinfo *cur;
	int error;
	char *cp, *hostname2 = NULL, *scope, *addr;
	struct sockaddr_in6 *sin6;

	_DIAGASSERT(pai != NULL);
	/* hostname may be NULL */
	/* servname may be NULL */
	_DIAGASSERT(res != NULL);

	/*
	 * if the servname does not match socktype/protocol, ignore it.
	 */
	if (get_portmatch(pai, servname, svd) != 0)
		return 0;

	afd = find_afd(pai->ai_family);
	if (afd == NULL)
		return 0;

	if (!afd->a_scoped)
		return explore_numeric(pai, hostname, servname, res, hostname,
		    svd);

	cp = strchr(hostname, SCOPE_DELIMITER);
	if (cp == NULL)
		return explore_numeric(pai, hostname, servname, res, hostname,
		    svd);

	/*
	 * Handle special case of <scoped_address><delimiter><scope id>
	 */
	hostname2 = strdup(hostname);
	if (hostname2 == NULL)
		return EAI_MEMORY;
	/* terminate at the delimiter */
	hostname2[cp - hostname] = '\0';
	addr = hostname2;
	scope = cp + 1;

	error = explore_numeric(pai, addr, servname, res, hostname, svd);
	if (error == 0) {
		u_int32_t scopeid;

		for (cur = *res; cur; cur = cur->ai_next) {
			if (cur->ai_family != AF_INET6)
				continue;
			sin6 = (struct sockaddr_in6 *)(void *)cur->ai_addr;
			if (ip6_str2scopeid(scope, sin6, &scopeid) == -1) {
				free(hostname2);
				return(EAI_NODATA); /* XXX: is return OK? */
			}
			sin6->sin6_scope_id = scopeid;
		}
	}

	free(hostname2);

	return error;
#endif
}

static int
get_canonname(const struct addrinfo *pai, struct addrinfo *ai, const char *str)
{

	_DIAGASSERT(pai != NULL);
	_DIAGASSERT(ai != NULL);
	_DIAGASSERT(str != NULL);

	if ((pai->ai_flags & AI_CANONNAME) != 0) {
		ai->ai_canonname = strdup(str);
		if (ai->ai_canonname == NULL)
			return EAI_MEMORY;
	}
	return 0;
}

static struct addrinfo *
get_ai(const struct addrinfo *pai, const struct afd *afd, const char *addr)
{
	char *p;
	struct addrinfo *ai;

	_DIAGASSERT(pai != NULL);
	_DIAGASSERT(afd != NULL);
	_DIAGASSERT(addr != NULL);

	ai = (struct addrinfo *)malloc(sizeof(struct addrinfo)
		+ (afd->a_socklen));
	if (ai == NULL)
		return NULL;

	memcpy(ai, pai, sizeof(struct addrinfo));
	ai->ai_addr = (struct sockaddr *)(void *)(ai + 1);
	memset(ai->ai_addr, 0, (size_t)afd->a_socklen);
	ai->ai_addr->sa_len = afd->a_socklen;
	ai->ai_addrlen = afd->a_socklen;
	ai->ai_addr->sa_family = ai->ai_family = afd->a_af;
	p = (char *)(void *)(ai->ai_addr);
	memcpy(p + afd->a_off, addr, (size_t)afd->a_addrlen);
	return ai;
}

static int
get_portmatch(const struct addrinfo *ai, const char *servname,
    struct servent_data *svd)
{

	_DIAGASSERT(ai != NULL);
	/* servname may be NULL */

	return get_port(ai, servname, 1, svd);
}

static int
get_port(const struct addrinfo *ai, const char *servname, int matchonly,
    struct servent_data *svd)
{
	const char *proto;
	struct servent *sp;
	int port;
	int allownumeric;

	_DIAGASSERT(ai != NULL);
	/* servname may be NULL */

	if (servname == NULL)
		return 0;
	switch (ai->ai_family) {
	case AF_INET:
#ifdef AF_INET6
	case AF_INET6:
#endif
		break;
	default:
		return 0;
	}

	switch (ai->ai_socktype) {
	case SOCK_RAW:
		return EAI_SERVICE;
	case SOCK_DGRAM:
	case SOCK_STREAM:
		allownumeric = 1;
		break;
	case ANY:
		/*
		 * This was 0.  It is now 1 so that queries specifying
		 * a NULL hint, or hint without socktype (but, hopefully,
		 * with protocol) and numeric address actually work.
		 */
		allownumeric = 1;
		break;
	default:
		return EAI_SOCKTYPE;
	}

	port = str2number(servname);
	if (port >= 0) {
		if (!allownumeric)
			return EAI_SERVICE;
		if (port < 0 || port > 65535)
			return EAI_SERVICE;
		port = htons(port);
	} else {
		struct servent sv = _svs_serv;
		if (ai->ai_flags & AI_NUMERICSERV)
			return EAI_NONAME;

		switch (ai->ai_socktype) {
		case SOCK_DGRAM:
			proto = "udp";
			break;
		case SOCK_STREAM:
			proto = "tcp";
			break;
		default:
			proto = NULL;
			break;
		}

        (void)getservbyname_r(&sv, svd, servname, proto, _svs_servbuf, sizeof(_svs_servbuf), &sp);
		if (sp == NULL)
			return EAI_SERVICE;
		port = sp->s_port;
	}

	if (!matchonly) {
		switch (ai->ai_family) {
		case AF_INET:
			((struct sockaddr_in *)(void *)
			    ai->ai_addr)->sin_port = port;
			break;
#ifdef INET6
		case AF_INET6:
			((struct sockaddr_in6 *)(void *)
			    ai->ai_addr)->sin6_port = port;
			break;
#endif
		}
	}

	return 0;
}

static const struct afd *
find_afd(int af)
{
	const struct afd *afd;

	if (af == PF_UNSPEC)
		return NULL;
	for (afd = afdl; afd->a_af; afd++) {
		if (afd->a_af == af)
			return afd;
	}
	return NULL;
}

#ifdef INET6
/* convert a string to a scope identifier. XXX: IPv6 specific */
static int
ip6_str2scopeid(char *scope, struct sockaddr_in6 *sin6, u_int32_t *scopeid)
{
	u_long lscopeid;
	struct in6_addr *a6;
	char *ep;

	_DIAGASSERT(scope != NULL);
	_DIAGASSERT(sin6 != NULL);
	_DIAGASSERT(scopeid != NULL);

	a6 = &sin6->sin6_addr;

	/* empty scopeid portion is invalid */
	if (*scope == '\0')
		return -1;

	if (IN6_IS_ADDR_LINKLOCAL(a6) || IN6_IS_ADDR_MC_LINKLOCAL(a6)) {
		/*
		 * We currently assume a one-to-one mapping between links
		 * and interfaces, so we simply use interface indices for
		 * like-local scopes.
		 */
		*scopeid = if_nametoindex(scope);
		if (*scopeid == 0)
			goto trynumeric;
		return 0;
	}

	/* still unclear about literal, allow numeric only - placeholder */
	if (IN6_IS_ADDR_SITELOCAL(a6) || IN6_IS_ADDR_MC_SITELOCAL(a6))
		goto trynumeric;
	if (IN6_IS_ADDR_MC_ORGLOCAL(a6))
		goto trynumeric;
	else
		goto trynumeric;	/* global */

	/* try to convert to a numeric id as a last resort */
  trynumeric:
	errno = 0;
	lscopeid = strtoul(scope, &ep, 10);
	*scopeid = (u_int32_t)(lscopeid & 0xffffffffUL);
	if (errno == 0 && ep && *ep == '\0' && *scopeid == lscopeid)
		return 0;
	else
		return -1;
}
#endif

/* code duplicate with gethnamaddr.c */

static const char AskedForGot[] =
	"gethostby*.getanswer: asked for \"%s\", got \"%s\"";

static struct addrinfo *
getanswer(const querybuf *answer, int anslen, const char *qname, int qtype,
    const struct addrinfo *pai)
{
	struct addrinfo sentinel, *cur;
	struct addrinfo ai;
	const struct afd *afd;
	char *canonname;
	const HEADER *hp;
	const u_char *cp;
	int n;
	const u_char *eom;
	char *bp, *ep;
	int type, class, ancount, qdcount;
	int haveanswer, had_error;
	char tbuf[MAXDNAME];
	int (*name_ok) (const char *);
	char hostbuf[8*1024];

	_DIAGASSERT(answer != NULL);
	_DIAGASSERT(qname != NULL);
	_DIAGASSERT(pai != NULL);

	memset(&sentinel, 0, sizeof(sentinel));
	cur = &sentinel;

	canonname = NULL;
	eom = answer->buf + anslen;
	switch (qtype) {
	case T_A:
	case T_AAAA:
	case T_ANY:	/*use T_ANY only for T_A/T_AAAA lookup*/
		name_ok = res_hnok;
		break;
	default:
		return NULL;	/* XXX should be abort(); */
	}
	/*
	 * find first satisfactory answer
	 */
	hp = &answer->hdr;
	ancount = ntohs(hp->ancount);
	qdcount = ntohs(hp->qdcount);
	bp = hostbuf;
	ep = hostbuf + sizeof hostbuf;
	cp = answer->buf + HFIXEDSZ;
	if (qdcount != 1) {
		h_errno = NO_RECOVERY;
		return (NULL);
	}
	n = dn_expand(answer->buf, eom, cp, bp, ep - bp);
	if ((n < 0) || !(*name_ok)(bp)) {
		h_errno = NO_RECOVERY;
		return (NULL);
	}
	cp += n + QFIXEDSZ;
	if (qtype == T_A || qtype == T_AAAA || qtype == T_ANY) {
		/* res_send() has already verified that the query name is the
		 * same as the one we sent; this just gets the expanded name
		 * (i.e., with the succeeding search-domain tacked on).
		 */
		n = strlen(bp) + 1;		/* for the \0 */
		if (n >= MAXHOSTNAMELEN) {
			h_errno = NO_RECOVERY;
			return (NULL);
		}
		canonname = bp;
		bp += n;
		/* The qname can be abbreviated, but h_name is now absolute. */
		qname = canonname;
	}
	haveanswer = 0;
	had_error = 0;
	while (ancount-- > 0 && cp < eom && !had_error) {
		n = dn_expand(answer->buf, eom, cp, bp, ep - bp);
		if ((n < 0) || !(*name_ok)(bp)) {
			had_error++;
			continue;
		}
		cp += n;			/* name */
		type = getshort(cp);
 		cp += INT16SZ;			/* type */
		class = getshort(cp);
 		cp += INT16SZ + INT32SZ;	/* class, TTL */
		n = getshort(cp);
		cp += INT16SZ;			/* len */
		if (class != C_IN) {
			/* XXX - debug? syslog? */
			cp += n;
			continue;		/* XXX - had_error++ ? */
		}
		if ((qtype == T_A || qtype == T_AAAA || qtype == T_ANY) &&
		    type == T_CNAME) {
			n = dn_expand(answer->buf, eom, cp, tbuf, sizeof tbuf);
			if ((n < 0) || !(*name_ok)(tbuf)) {
				had_error++;
				continue;
			}
			cp += n;
			/* Get canonical name. */
			n = strlen(tbuf) + 1;	/* for the \0 */
			if (n > ep - bp || n >= MAXHOSTNAMELEN) {
				had_error++;
				continue;
			}
			strlcpy(bp, tbuf, (size_t)(ep - bp));
			canonname = bp;
			bp += n;
			continue;
		}
		if (qtype == T_ANY) {
			if (!(type == T_A || type == T_AAAA)) {
				cp += n;
				continue;
			}
		} else if (type != qtype) {
			if (type != T_KEY && type != T_SIG) {
				syslog(LOG_NOTICE|LOG_AUTH,
	       "gethostby*.getanswer: asked for \"%s %s %s\", got type \"%s\"",
				       qname, p_class(C_IN), p_type(qtype),
				       p_type(type));
			}
			cp += n;
			continue;		/* XXX - had_error++ ? */
		}
		switch (type) {
		case T_A:
		case T_AAAA:
			if (strcasecmp(canonname, bp) != 0) {
				syslog(LOG_NOTICE|LOG_AUTH, AskedForGot, canonname, bp);
				cp += n;
				continue;	/* XXX - had_error++ ? */
			}
			if (type == T_A && n != INADDRSZ) {
				cp += n;
				continue;
			}
			if (type == T_AAAA && n != IN6ADDRSZ) {
				cp += n;
				continue;
			}
			if (type == T_AAAA) {
				struct in6_addr in6;
				memcpy(&in6, cp, IN6ADDRSZ);
				if (IN6_IS_ADDR_V4MAPPED(&in6)) {
					cp += n;
					continue;
				}
			}
			if (!haveanswer) {
				int nn;

				canonname = bp;
				nn = strlen(bp) + 1;	/* for the \0 */
				bp += nn;
			}

			/* don't overwrite pai */
			ai = *pai;
			ai.ai_family = (type == T_A) ? AF_INET : AF_INET6;
			afd = find_afd(ai.ai_family);
			if (afd == NULL) {
				cp += n;
				continue;
			}
			cur->ai_next = get_ai(&ai, afd, (const char *)cp);
			if (cur->ai_next == NULL)
				had_error++;
			while (cur && cur->ai_next)
				cur = cur->ai_next;
			cp += n;
			break;
		default:
			abort();
		}
		if (!had_error)
			haveanswer++;
	}
	if (haveanswer) {
		if (!canonname)
			(void)get_canonname(pai, sentinel.ai_next, qname);
		else
			(void)get_canonname(pai, sentinel.ai_next, canonname);
		h_errno = NETDB_SUCCESS;
		return sentinel.ai_next;
	}

	h_errno = NO_RECOVERY;
	return NULL;
}

#define SORTEDADDR(p)	(((struct sockaddr_in *)(void *)(p->ai_next->ai_addr))->sin_addr.s_addr)
#define SORTMATCH(p, s) ((SORTEDADDR(p) & (s).mask) == (s).addr.s_addr)

static void
aisort(struct addrinfo *s, res_state res)
{
	struct addrinfo head, *t, *p;
	int i;

	head.ai_next = NULL;
	t = &head;

	for (i = 0; i < res->nsort; i++) {
		p = s;
		while (p->ai_next) {
			if ((p->ai_next->ai_family != AF_INET)
			|| SORTMATCH(p, res->sort_list[i])) {
				t->ai_next = p->ai_next;
				t = t->ai_next;
				p->ai_next = p->ai_next->ai_next;
			} else {
				p = p->ai_next;
			}
		}
	}

	/* add rest of list and reset s to the new list*/
	t->ai_next = s->ai_next;
	s->ai_next = head.ai_next;
}

/*ARGSUSED*/
static int
_dns_getaddrinfo(void *rv, void	*cb_data, va_list ap)
{
	struct addrinfo *ai;
	querybuf *buf, *buf2;
	const char *name;
	const struct addrinfo *pai;
	struct addrinfo sentinel, *cur;
	struct res_target q, q2;
	res_state res;

	name = va_arg(ap, char *);
	pai = va_arg(ap, const struct addrinfo *);

	memset(&q, 0, sizeof(q));
	memset(&q2, 0, sizeof(q2));
	memset(&sentinel, 0, sizeof(sentinel));
	cur = &sentinel;

	buf = malloc(sizeof(*buf));
	if (buf == NULL) {
		h_errno = NETDB_INTERNAL;
		return NS_NOTFOUND;
	}
	buf2 = malloc(sizeof(*buf2));
	if (buf2 == NULL) {
		free(buf);
		h_errno = NETDB_INTERNAL;
		return NS_NOTFOUND;
	}

	switch (pai->ai_family) {
	case AF_UNSPEC:
		/* prefer IPv6 */
		q.name = name;
		q.qclass = C_IN;
		q.qtype = T_AAAA;
		q.answer = buf->buf;
		q.anslen = sizeof(buf->buf);
		q.next = &q2;
		q2.name = name;
		q2.qclass = C_IN;
		q2.qtype = T_A;
		q2.answer = buf2->buf;
		q2.anslen = sizeof(buf2->buf);
		break;
	case AF_INET:
		q.name = name;
		q.qclass = C_IN;
		q.qtype = T_A;
		q.answer = buf->buf;
		q.anslen = sizeof(buf->buf);
		break;
	case AF_INET6:
		q.name = name;
		q.qclass = C_IN;
		q.qtype = T_AAAA;
		q.answer = buf->buf;
		q.anslen = sizeof(buf->buf);
		break;
	default:
		free(buf);
		free(buf2);
		return NS_UNAVAIL;
	}

	res = res_get_state();
	if (res == NULL) {
		free(buf);
		free(buf2);
		return NS_NOTFOUND;
	}

	if (res_searchN(name, &q, res) < 0) {
		res_put_state(res);
		free(buf);
		free(buf2);
		return NS_NOTFOUND;
	}
	ai = getanswer(buf, q.n, q.name, q.qtype, pai);
	if (ai) {
		cur->ai_next = ai;
		while (cur && cur->ai_next)
			cur = cur->ai_next;
	}
	if (q.next) {
		ai = getanswer(buf2, q2.n, q2.name, q2.qtype, pai);
		if (ai)
			cur->ai_next = ai;
	}
	free(buf);
	free(buf2);
	if (sentinel.ai_next == NULL) {
		res_put_state(res);
		switch (h_errno) {
		case HOST_NOT_FOUND:
			return NS_NOTFOUND;
		case TRY_AGAIN:
			return NS_TRYAGAIN;
		default:
			return NS_UNAVAIL;
		}
	}

	if (res->nsort)
		aisort(&sentinel, res);

	res_put_state(res);

	*((struct addrinfo **)rv) = sentinel.ai_next;
	return NS_SUCCESS;
}

static void
_sethtent(FILE **hostf)
{

	if (!*hostf)
		*hostf = fopen(_PATH_HOSTS, "r" );
	else
		rewind(*hostf);
}

static void
_endhtent(FILE **hostf)
{

	if (*hostf) {
		(void) fclose(*hostf);
		*hostf = NULL;
	}
}

static struct addrinfo *
_gethtent(FILE **hostf, const char *name, const struct addrinfo *pai)
{
	char *p;
	char *cp, *tname, *cname;
	struct addrinfo hints, *res0, *res;
	int error;
	const char *addr;
	char hostbuf[8*1024];

	_DIAGASSERT(name != NULL);
	_DIAGASSERT(pai != NULL);

	if (!*hostf && !(*hostf = fopen(_PATH_HOSTS, "r" )))
		return (NULL);
 again:
	if (!(p = fgets(hostbuf, sizeof hostbuf, *hostf)))
		return (NULL);
	if (*p == '#')
		goto again;
	if (!(cp = strpbrk(p, "#\n")))
		goto again;
	*cp = '\0';
	if (!(cp = strpbrk(p, " \t")))
		goto again;
	*cp++ = '\0';
	addr = p;
	/* if this is not something we're looking for, skip it. */
	cname = NULL;
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (!cname)
			cname = cp;
		tname = cp;
		if ((cp = strpbrk(cp, " \t")) != NULL)
			*cp++ = '\0';
		if (strcasecmp(name, tname) == 0)
			goto found;
	}
	goto again;

found:
	hints = *pai;
	hints.ai_flags = AI_NUMERICHOST;
	error = getaddrinfo(addr, NULL, &hints, &res0);
	if (error)
		goto again;
	for (res = res0; res; res = res->ai_next) {
		/* cover it up */
		res->ai_flags = pai->ai_flags;

		if (pai->ai_flags & AI_CANONNAME) {
			if (get_canonname(pai, res, cname) != 0) {
				freeaddrinfo(res0);
				goto again;
			}
		}
	}
	return res0;
}

/*ARGSUSED*/
static int
_files_getaddrinfo(void *rv, void *cb_data, va_list ap)
{
	const char *name;
	const struct addrinfo *pai;
	struct addrinfo sentinel, *cur;
	struct addrinfo *p;
#ifndef _REENTRANT
	static
#endif
	FILE *hostf = NULL;

	name = va_arg(ap, char *);
	pai = va_arg(ap, const struct addrinfo *);

	memset(&sentinel, 0, sizeof(sentinel));
	cur = &sentinel;

	_sethtent(&hostf);
	while ((p = _gethtent(&hostf, name, pai)) != NULL) {
		cur->ai_next = p;
		while (cur && cur->ai_next)
			cur = cur->ai_next;
	}
	_endhtent(&hostf);

	*((struct addrinfo **)rv) = sentinel.ai_next;
	if (sentinel.ai_next == NULL)
		return NS_NOTFOUND;
	return NS_SUCCESS;
}

#ifdef YP
/*ARGSUSED*/
static struct addrinfo *
_yphostent(char *line, const struct addrinfo *pai)
{
	struct addrinfo sentinel, *cur;
	struct addrinfo hints, *res, *res0;
	int error;
	char *p;
	const char *addr, *canonname;
	char *nextline;
	char *cp;

	_DIAGASSERT(line != NULL);
	_DIAGASSERT(pai != NULL);

	p = line;
	addr = canonname = NULL;

	memset(&sentinel, 0, sizeof(sentinel));
	cur = &sentinel;

nextline:
	/* terminate line */
	cp = strchr(p, '\n');
	if (cp) {
		*cp++ = '\0';
		nextline = cp;
	} else
		nextline = NULL;

	cp = strpbrk(p, " \t");
	if (cp == NULL) {
		if (canonname == NULL)
			return (NULL);
		else
			goto done;
	}
	*cp++ = '\0';

	addr = p;

	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (!canonname)
			canonname = cp;
		if ((cp = strpbrk(cp, " \t")) != NULL)
			*cp++ = '\0';
	}

	hints = *pai;
	hints.ai_flags = AI_NUMERICHOST;
	error = getaddrinfo(addr, NULL, &hints, &res0);
	if (error == 0) {
		for (res = res0; res; res = res->ai_next) {
			/* cover it up */
			res->ai_flags = pai->ai_flags;

			if (pai->ai_flags & AI_CANONNAME)
				(void)get_canonname(pai, res, canonname);
		}
	} else
		res0 = NULL;
	if (res0) {
		cur->ai_next = res0;
		while (cur->ai_next)
			cur = cur->ai_next;
	}

	if (nextline) {
		p = nextline;
		goto nextline;
	}

done:
	return sentinel.ai_next;
}

/*ARGSUSED*/
static int
_yp_getaddrinfo(void *rv, void *cb_data, va_list ap)
{
	struct addrinfo sentinel, *cur;
	struct addrinfo *ai = NULL;
	char *ypbuf;
	int ypbuflen, r;
	const char *name;
	const struct addrinfo *pai;
	char *ypdomain;

	if (_yp_check(&ypdomain) == 0)
		return NS_UNAVAIL;

	name = va_arg(ap, char *);
	pai = va_arg(ap, const struct addrinfo *);

	memset(&sentinel, 0, sizeof(sentinel));
	cur = &sentinel;

	/* hosts.byname is only for IPv4 (Solaris8) */
	if (pai->ai_family == PF_UNSPEC || pai->ai_family == PF_INET) {
		r = yp_match(ypdomain, "hosts.byname", name,
			(int)strlen(name), &ypbuf, &ypbuflen);
		if (r == 0) {
			struct addrinfo ai4;

			ai4 = *pai;
			ai4.ai_family = AF_INET;
			ai = _yphostent(ypbuf, &ai4);
			if (ai) {
				cur->ai_next = ai;
				while (cur && cur->ai_next)
					cur = cur->ai_next;
			}
		}
		free(ypbuf);
	}

	/* ipnodes.byname can hold both IPv4/v6 */
	r = yp_match(ypdomain, "ipnodes.byname", name,
		(int)strlen(name), &ypbuf, &ypbuflen);
	if (r == 0) {
		ai = _yphostent(ypbuf, pai);
		if (ai)
			cur->ai_next = ai;
		free(ypbuf);
	}

	if (sentinel.ai_next == NULL) {
		h_errno = HOST_NOT_FOUND;
		return NS_NOTFOUND;
	}
	*((struct addrinfo **)rv) = sentinel.ai_next;
	return NS_SUCCESS;
}
#endif

/* resolver logic */

/*
 * Formulate a normal query, send, and await answer.
 * Returned answer is placed in supplied buffer "answer".
 * Perform preliminary check of answer, returning success only
 * if no error is indicated and the answer count is nonzero.
 * Return the size of the response on success, -1 on error.
 * Error number is left in h_errno.
 *
 * Caller must parse answer and determine whether it answers the question.
 */
static int
res_queryN(const char *name, /* domain name */ struct res_target *target, res_state res)
{
	u_char buf[MAXPACKET];
	HEADER *hp;
	int n;
	struct res_target *t;
	int rcode;
	int ancount;

	_DIAGASSERT(name != NULL);
	/* XXX: target may be NULL??? */

	rcode = NOERROR;
	ancount = 0;

	for (t = target; t; t = t->next) {
		int class, type;
		u_char *answer;
		int anslen;

		hp = (HEADER *)(void *)t->answer;
		hp->rcode = NOERROR;	/* default */

		/* make it easier... */
		class = t->qclass;
		type = t->qtype;
		answer = t->answer;
		anslen = t->anslen;
#ifdef DEBUG
		if (res->options & RES_DEBUG)
			printf(";; res_nquery(%s, %d, %d)\n", name, class, type);
#endif

		n = res_nmkquery(res, QUERY, name, class, type, NULL, 0, NULL, buf, sizeof(buf));
#ifdef RES_USE_EDNS0
		if (n > 0 && (res->options & RES_USE_EDNS0) != 0)
			n = res_nopt(res, n, buf, sizeof(buf), anslen);
#endif
		if (n <= 0) {
#ifdef DEBUG
			if (res->options & RES_DEBUG)
				printf(";; res_nquery: mkquery failed\n");
#endif
			h_errno = NO_RECOVERY;
			return n;
		}
		n = res_nsend(res, buf, n, answer, anslen);
#if 0
		if (n < 0) {
#ifdef DEBUG
			if (res->options & RES_DEBUG)
				printf(";; res_query: send error\n");
#endif
			h_errno = TRY_AGAIN;
			return n;
		}
#endif

		if (n < 0 || hp->rcode != NOERROR || ntohs(hp->ancount) == 0) {
			rcode = hp->rcode;	/* record most recent error */
#ifdef DEBUG
			if (res->options & RES_DEBUG)
				printf(";; rcode = %u, ancount=%u\n", hp->rcode,
				    ntohs(hp->ancount));
#endif
			continue;
		}

		ancount += ntohs(hp->ancount);

		t->n = n;
	}

	if (ancount == 0) {
		switch (rcode) {
		case NXDOMAIN:
			h_errno = HOST_NOT_FOUND;
			break;
		case SERVFAIL:
			h_errno = TRY_AGAIN;
			break;
		case NOERROR:
			h_errno = NO_DATA;
			break;
		case FORMERR:
		case NOTIMP:
		case REFUSED:
		default:
			h_errno = NO_RECOVERY;
			break;
		}
		return -1;
	}
	return ancount;
}

/*
 * Formulate a normal query, send, and retrieve answer in supplied buffer.
 * Return the size of the response on success, -1 on error.
 * If enabled, implement search rules until answer or unrecoverable failure
 * is detected.  Error code, if any, is left in h_errno.
 */
static int
res_searchN(const char *name, struct res_target *target, res_state res)
{
	const char *cp, * const *domain;
	HEADER *hp;
	u_int dots;
	int trailing_dot, ret, saved_herrno;
	int got_nodata = 0, got_servfail = 0, tried_as_is = 0;

	_DIAGASSERT(name != NULL);
	_DIAGASSERT(target != NULL);

	hp = (HEADER *)(void *)target->answer;	/*XXX*/

	errno = 0;
	h_errno = HOST_NOT_FOUND;	/* default, if we never query */
	dots = 0;
	for (cp = name; *cp; cp++)
		dots += (*cp == '.');
	trailing_dot = 0;
	if (cp > name && *--cp == '.')
		trailing_dot++;

	/*
	 * if there aren't any dots, it could be a user-level alias
	 */
	if (!dots && (cp = hostalias(name)) != NULL) {
		ret = res_queryN(cp, target, res);
		return ret;
	}

	/*
	 * If there are dots in the name already, let's just give it a try
	 * 'as is'.  The threshold can be set with the "ndots" option.
	 */
	saved_herrno = -1;
	if (dots >= res->ndots) {
		ret = res_querydomainN(name, NULL, target, res);
		if (ret > 0)
			return (ret);
		saved_herrno = h_errno;
		tried_as_is++;
	}

	/*
	 * We do at least one level of search if
	 *	- there is no dot and RES_DEFNAME is set, or
	 *	- there is at least one dot, there is no trailing dot,
	 *	  and RES_DNSRCH is set.
	 */
	if ((!dots && (res->options & RES_DEFNAMES)) ||
	    (dots && !trailing_dot && (res->options & RES_DNSRCH))) {
		int done = 0;

		for (domain = (const char * const *)res->dnsrch;
		   *domain && !done;
		   domain++) {

			ret = res_querydomainN(name, *domain, target, res);
			if (ret > 0)
				return ret;

			/*
			 * If no server present, give up.
			 * If name isn't found in this domain,
			 * keep trying higher domains in the search list
			 * (if that's enabled).
			 * On a NO_DATA error, keep trying, otherwise
			 * a wildcard entry of another type could keep us
			 * from finding this entry higher in the domain.
			 * If we get some other error (negative answer or
			 * server failure), then stop searching up,
			 * but try the input name below in case it's
			 * fully-qualified.
			 */
			if (errno == ECONNREFUSED) {
				h_errno = TRY_AGAIN;
				return -1;
			}

			switch (h_errno) {
			case NO_DATA:
				got_nodata++;
				/* FALLTHROUGH */
			case HOST_NOT_FOUND:
				/* keep trying */
				break;
			case TRY_AGAIN:
				if (hp->rcode == SERVFAIL) {
					/* try next search element, if any */
					got_servfail++;
					break;
				}
				/* FALLTHROUGH */
			default:
				/* anything else implies that we're done */
				done++;
			}
			/*
			 * if we got here for some reason other than DNSRCH,
			 * we only wanted one iteration of the loop, so stop.
			 */
			if (!(res->options & RES_DNSRCH))
			        done++;
		}
	}

	/*
	 * if we have not already tried the name "as is", do that now.
	 * note that we do this regardless of how many dots were in the
	 * name or whether it ends with a dot.
	 */
	if (!tried_as_is) {
		ret = res_querydomainN(name, NULL, target, res);
		if (ret > 0)
			return ret;
	}

	/*
	 * if we got here, we didn't satisfy the search.
	 * if we did an initial full query, return that query's h_errno
	 * (note that we wouldn't be here if that query had succeeded).
	 * else if we ever got a nodata, send that back as the reason.
	 * else send back meaningless h_errno, that being the one from
	 * the last DNSRCH we did.
	 */
	if (saved_herrno != -1)
		h_errno = saved_herrno;
	else if (got_nodata)
		h_errno = NO_DATA;
	else if (got_servfail)
		h_errno = TRY_AGAIN;
	return -1;
}

/*
 * Perform a call on res_query on the concatenation of name and domain,
 * removing a trailing dot from name if domain is NULL.
 */
static int
res_querydomainN(const char *name, const char *domain,
    struct res_target *target, res_state res)
{
	char nbuf[MAXDNAME];
	const char *longname = nbuf;
	size_t n, d;

	_DIAGASSERT(name != NULL);
	/* XXX: target may be NULL??? */

#ifdef DEBUG
	if (res->options & RES_DEBUG)
		printf(";; res_querydomain(%s, %s)\n",
			name, domain?domain:"<Nil>");
#endif
	if (domain == NULL) {
		/*
		 * Check for trailing '.';
		 * copy without '.' if present.
		 */
		n = strlen(name);
		if (n >= sizeof(nbuf)) {
			h_errno = NO_RECOVERY;
			return -1;
		}
		if (n > 0 && name[--n] == '.') {
            //strncpy(nbuf, name, n);
			//nbuf[n] = '\0';
            snprintf(nbuf, sizeof(nbuf), "%*s", (int)n, name);
		} else
			longname = name;
	} else {
		n = strlen(name);
		d = strlen(domain);
		if (n + 1 + d + 1 > sizeof(nbuf)) {
			h_errno = NO_RECOVERY;
			return -1;
		}
		snprintf(nbuf, sizeof(nbuf), "%s.%s", name, domain);
	}
	return res_queryN(longname, target, res);
}
