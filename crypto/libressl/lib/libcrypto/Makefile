#	$211BSD: Makefile,v 1.0 2025/03/31 19:15:27 Exp $
#   Makefile LibreSSL 4.0 libcrypto

LIB=	crypto
USE_FORT?= yes	# cryptographic software
USE_SHLIBDIR=	yes
#DBG=-g

.include <bsd.own.mk>
.include <bsd.shlib.mk>

WARNS=0

LCRYPTO_SRC=  ${LIBRESSLSRC}/crypto
LCRYPTO_INC=  ${LIBRESSLSRC}/include

CPPFLAGS+= -DLIBRESSL_INTERNAL
CPPFLAGS+= -DLIBRESSL_NAMESPACE -DLIBRESSL_CRYPTO_NAMESPACE
CPPFLAGS+= -Dlib${LIB} -I.
CPPFLAGS+= -I${LIBRESSLSRC}
CPPFLAGS+= -I${LCRYPTO_SRC}
CPPFLAGS+= -I${LCRYPTO_SRC}/hidden
CPPFLAGS+= -I${LCRYPTO_SRC}/compat

CRYPTODIST=	${NETBSDSRCDIR}/crypto

.include "${NETBSDSRCDIR}/crypto/Makefile.libressl"
.PATH: ${LIBRESSLSRC}
.PATH: ${LCRYPTO_SRC}
.PATH: ${LCRYPTO_SRC}/hidden
.PATH: ${LCRYPTO_SRC}/compat

AFLAGS+=-DELF
LIBDPLIBS+= crypt ${NETBSDSRCDIR}/usr.lib/libcrypt

.include "Makefile.incs"
.include "Makefile.crypto"
INCSDIR= /usr/include/openssl

PKGCONFIG= libcrypto
.include "${.CURDIR}/../../pkgconfig.mk"

.include <bsd.lib.mk>

COPTS.chacha.c += -Wno-attributes
COPTS.chacha-merged.c += -Wno-attributes
