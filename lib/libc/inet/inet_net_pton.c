/*	$NetBSD: inet_net_pton.c,v 1.16 2001/12/08 12:06:12 lukem Exp $	*/

/*
 * Copyright (c) 1996,1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static const char rcsid[] = "Id: inet_net_pton.c,v 1.13 2001/09/27 15:08:38 marka Exp ";
#else
__RCSID("$NetBSD: inet_net_pton.c,v 1.16 2001/12/08 12:06:12 lukem Exp $");
#endif
#endif

#include "namespace.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __weak_alias
__weak_alias(inet_net_pton,_inet_net_pton)
#endif

static int	inet_net_pton_ipv4(const char *, u_char *, size_t);
static int	inet_net_pton_ipv6(const char *, u_char *, size_t);
static int	getbits(const char *, int *);
static int	getv4(const char *, u_char *, int *);

/*
 * static int
 * inet_net_pton(af, src, dst, size)
 *	convert network number from presentation to network format.
 *	accepts hex octets, hex strings, decimal octets, and /CIDR.
 *	"size" is in bytes and describes "dst".
 * return:
 *	number of bits, either imputed classfully or specified with /CIDR,
 *	or -1 if some failure occurred (check errno).  ENOENT means it was
 *	not a valid network specification.
 * author:
 *	Paul Vixie (ISC), June 1996
 */
int
inet_net_pton(int af, const char *src, void *dst, size_t size)
{

	_DIAGASSERT(src != NULL);
	_DIAGASSERT(dst != NULL);

	switch (af) {
	case AF_INET:
		return (inet_net_pton_ipv4(src, dst, size));
	case AF_INET6:
		return (inet_net_pton_ipv6(src, dst, size));
	default:
		errno = EAFNOSUPPORT;
		return (-1);
	}
}

/*
 * static int
 * inet_net_pton_ipv4(src, dst, size)
 *	convert IPv4 network number from presentation to network format.
 *	accepts hex octets, hex strings, decimal octets, and /CIDR.
 *	"size" is in bytes and describes "dst".
 * return:
 *	number of bits, either imputed classfully or specified with /CIDR,
 *	or -1 if some failure occurred (check errno).  ENOENT means it was
 *	not an IPv4 network specification.
 * note:
 *	network byte order assumed.  this means 192.5.5.240/28 has
 *	0b11110000 in its fourth octet.
 * author:
 *	Paul Vixie (ISC), June 1996
 */
static int
inet_net_pton_ipv4(const char *src, u_char *dst, size_t size)
{
	static const char
		xdigits[] = "0123456789abcdef",
		digits[] = "0123456789";
	int n, ch, tmp = 0, dirty, bits;
	const u_char *odst = dst;

	_DIAGASSERT(src != NULL);
	_DIAGASSERT(dst != NULL);

	ch = (u_char) *src++;
	if (ch == '0' && (src[0] == 'x' || src[0] == 'X')
	    && isascii((u_char) src[1]) && isxdigit((u_char) src[1])) {
		/* Hexadecimal: Eat nybble string. */
		/* size is unsigned */
		if (size == 0)
			goto emsgsize;
		dirty = 0;
		src++;	/* skip x or X. */
		while ((ch = (u_char) *src++) != '\0' && isascii(ch)
		    && isxdigit(ch)) {
			if (isupper(ch))
				ch = tolower(ch);
			n = strchr(xdigits, ch) - xdigits;
			assert(n >= 0 && n <= 15);
			if (dirty == 0)
				tmp = n;
			else
				tmp = (tmp << 4) | n;
			if (++dirty == 2) {
				if (size-- == 0)
					goto emsgsize;
				*dst++ = (u_char) tmp;
				dirty = 0;
			}
		}
		if (dirty) {  /* Odd trailing nybble? */
			if (size-- == 0)
				goto emsgsize;
			*dst++ = (u_char) (tmp << 4);
		}
	} else if (isascii(ch) && isdigit(ch)) {
		/* Decimal: eat dotted digit string. */
		for (;;) {
			tmp = 0;
			do {
				n = strchr(digits, ch) - digits;
				assert(n >= 0 && n <= 9);
				tmp *= 10;
				tmp += n;
				if (tmp > 255)
					goto enoent;
			} while ((ch = (u_char) *src++) != '\0' &&
				 isascii(ch) && isdigit(ch));
			if (size-- == 0)
				goto emsgsize;
			*dst++ = (u_char) tmp;
			if (ch == '\0' || ch == '/')
				break;
			if (ch != '.')
				goto enoent;
			ch = *src++;
			if (!isascii(ch) || !isdigit(ch))
				goto enoent;
		}
	} else
		goto enoent;

	bits = -1;
	if (ch == '/' && isascii((u_char) src[0]) && isdigit((u_char) src[0])
	    && dst > odst) {
		/* CIDR width specifier.  Nothing can follow it. */
		ch = (u_char) *src++;	/* Skip over the /. */
		bits = 0;
		do {
			n = strchr(digits, ch) - digits;
			assert(n >= 0 && n <= 9);
			bits *= 10;
			bits += n;
		} while ((ch = (u_char) *src++) != '\0' && isascii(ch)
		    && isdigit(ch));
		if (ch != '\0')
			goto enoent;
		if (bits > 32)
			goto emsgsize;
	}

	/* Firey death and destruction unless we prefetched EOS. */
	if (ch != '\0')
		goto enoent;

	/* If nothing was written to the destination, we found no address. */
	if (dst == odst)
		goto enoent;
	/* If no CIDR spec was given, infer width from net class. */
	if (bits == -1) {
		if (*odst >= 240)	/* Class E */
			bits = 32;
		else if (*odst >= 224)	/* Class D */
			bits = 4;
		else if (*odst >= 192)	/* Class C */
			bits = 24;
		else if (*odst >= 128)	/* Class B */
			bits = 16;
		else			/* Class A */
			bits = 8;
		/* If imputed mask is narrower than specified octets, widen. */
		if (bits >= 8 && bits < ((dst - odst) * 8))
			bits = (dst - odst) * 8;
	}
	/* Extend network to cover the actual mask. */
	while (bits > ((dst - odst) * 8)) {
		/* size is unsigned */
		if (size-- == 0)
			goto emsgsize;
		*dst++ = '\0';
	}
	return (bits);

 enoent:
	errno = ENOENT;
	return (-1);

 emsgsize:
	errno = EMSGSIZE;
	return (-1);
}

static int
getbits(const char *src, int *bitsp)
{
	static const char digits[] = "0123456789";
	int n;
	int val;
	char ch;

	val = 0;
	n = 0;
	while ((ch = *src++) != '\0') {
		const char *pch;

		pch = strchr(digits, ch);
		if (pch != NULL) {
			if (n++ != 0 && val == 0)	/* no leading zeros */
				return (0);
			val *= 10;
			val += (pch - digits);
			if (val > 128)			/* range */
				return (0);
			continue;
		}
		return (0);
	}
	if (n == 0)
		return (0);
	*bitsp = val;
	return (1);
}

static int
getv4(const char *src, u_char *dst, int *bitsp)
{
	static const char digits[] = "0123456789";
	u_char *odst = dst;
	int n;
	u_int val;
	char ch;

	val = 0;
	n = 0;
	while ((ch = *src++) != '\0') {
		const char *pch;

		pch = strchr(digits, ch);
		if (pch != NULL) {
			if (n++ != 0 && val == 0)	/* no leading zeros */
				return (0);
			val *= 10;
			val += (pch - digits);
			if (val > 255)			/* range */
				return (0);
			continue;
		}
		if (ch == '.' || ch == '/') {
			if (dst - odst > 3)		/* too many octets? */
				return (0);
			*dst++ = val;
			if (ch == '/')
				return (getbits(src, bitsp));
			val = 0;
			n = 0;
			continue;
		}
		return (0);
	}
	if (n == 0)
		return (0);
	if (dst - odst > 3)		/* too many octets? */
		return (0);
	*dst++ = val;
	return (1);
}

static int
inet_net_pton_ipv6(const char *src, u_char *dst, size_t size)
{
	static const char xdigits_l[] = "0123456789abcdef",
			  xdigits_u[] = "0123456789ABCDEF";
	u_char tmp[IN6ADDRSZ], *tp, *endp, *colonp;
	const char *xdigits, *curtok;
	int ch, saw_xdigit;
	u_int val;
	int digits;
	int bits;
	size_t bytes;
	int words;
	int ipv4;

	_DIAGASSERT(src != NULL);
	_DIAGASSERT(dst != NULL);

	memset((tp = tmp), '\0', IN6ADDRSZ);
	endp = tp + IN6ADDRSZ;
	colonp = NULL;
	/* Leading :: requires some special handling. */
	if (*src == ':')
		if (*++src != ':')
			goto enoent;
	curtok = src;
	saw_xdigit = 0;
	val = 0;
	digits = 0;
	bits = -1;
	ipv4 = 0;
	while ((ch = *src++) != '\0') {
		const char *pch;

		if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
			pch = strchr((xdigits = xdigits_u), ch);
		if (pch != NULL) {
			val <<= 4;
			val |= (pch - xdigits);
			if (++digits > 4)
				goto enoent;
			saw_xdigit = 1;
			continue;
		}
		if (ch == ':') {
			curtok = src;
			if (!saw_xdigit) {
				if (colonp)
					goto enoent;
				colonp = tp;
				continue;
			} else if (*src == '\0')
				goto enoent;
			if (tp + INT16SZ > endp)
				return (0);
			*tp++ = (u_char) (val >> 8) & 0xff;
			*tp++ = (u_char) val & 0xff;
			saw_xdigit = 0;
			digits = 0;
			val = 0;
			continue;
		}
		if (ch == '.' && ((tp + INADDRSZ) <= endp) &&
		     getv4(curtok, tp, &bits) > 0) {
			tp += INADDRSZ;
			saw_xdigit = 0;
			ipv4 = 1;
			break;	/* '\0' was seen by inet_pton4(). */
		}
		if (ch == '/' && getbits(src, &bits) > 0)
			break;
		goto enoent;
	}
	if (saw_xdigit) {
		if (tp + INT16SZ > endp)
			goto enoent;
		*tp++ = (u_char) (val >> 8) & 0xff;
		*tp++ = (u_char) val & 0xff;
	}
	if (bits == -1)
		bits = 128;

	words = (bits + 15) / 16;
	if (words < 2)
		words = 2;
	if (ipv4)
		words = 8;
	endp =  tmp + 2 * words;

	if (colonp != NULL) {
		/*
		 * Since some memmove()'s erroneously fail to handle
		 * overlapping regions, we'll do the shift by hand.
		 */
		const int n = tp - colonp;
		int i;

		if (tp == endp)
			goto enoent;
		for (i = 1; i <= n; i++) {
			endp[- i] = colonp[n - i];
			colonp[n - i] = 0;
		}
		tp = endp;
	}
	if (tp != endp)
		goto enoent;

	bytes = (bits + 7) / 8;
	if (bytes > size)
		goto emsgsize;
	memcpy(dst, tmp, bytes);
	return (bits);

 enoent:
	errno = ENOENT;
	return (-1);

 emsgsize:
	errno = EMSGSIZE;
	return (-1);
}
