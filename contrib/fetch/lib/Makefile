# $NetBSD: Makefile,v 1.11.6.1 2019/09/01 10:44:23 martin Exp $

LIB=		fetch
SRCS=		fetch.c common.c ftp.c http.c file.c
DPSRCS= 	ftperr.h httperr.h
INCS=		fetch.h
INCSDIR=	/usr/include
MAN=		fetch.3
CLEANFILES=	ftperr.h httperr.h

.include <bsd.init.mk>

CPPFLAGS+=	-I.
CPPFLAGS+=	-DNETBSD
CPPFLAGS+=	-D__SELECT_DECLARED
CPPFLAGS+=	-DFTP_COMBINE_CWDS
CPPFLAGS+=	-DINET6

CPPFLAGS+=	-DWITH_SSL

LIBDPLIBS+=	ssl	${NETBSDSRCDIR}/crypto/${EXTERNAL_OPENSSL_SUBDIR}/lib/libssl \
		crypto	${NETBSDSRCDIR}/crypto/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto

CLEANFILES+=	ftperr.h httperr.h

.PATH: ${LIBFETCHDIR}

ftperr.h: ${LIBFETCHDIR}/ftp.errors ${LIBFETCHDIR}/errlist.sh
	${HOST_SH} ${LIBFETCHDIR}/errlist.sh ftp_errlist FTP \
	    ${LIBFETCHDIR}/ftp.errors > ${.TARGET}

httperr.h: ${LIBFETCHDIR}/http.errors ${LIBFETCHDIR}/errlist.sh
	${HOST_SH} ${LIBFETCHDIR}/errlist.sh http_errlist HTTP \
	    ${LIBFETCHDIR}/http.errors > ${.TARGET}

.if ${MKSANITIZER:Uno} == "yes"
CFLAGS+=	-Wno-macro-redefined # _REENTRANT redefined
.endif

COPTS.ftp.c+= -Wno-sign-compare
COPTS.http.c+= -Wno-stringop-truncation

.include <bsd.lib.mk>
