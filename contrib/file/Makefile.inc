#	$NetBSD: Makefile.inc,v 1.7 2019/12/17 18:59:39 christos Exp $

WARNS=4
DIST=${NETBSDSRCDIR}/contrib/file/dist

.include <bsd.own.mk>

BINDIR?= /usr/bin
USE_FORT?= yes	# data-driven bugs?

TOOL_MKMAGIC?=	${.OBJDIR}/file

MFILESDIR?=	/usr/share/misc
MFILES?=	magic.mgc
MAGIC?=		${MFILESDIR}/magic

VERSION != ${TOOL_SED} -ne "s/^PACKAGE_VERSION='\([^']*\)'/\1/p" ${DIST}/configure

CPPFLAGS+=	-DMAGIC='"${MAGIC}"' -DHAVE_CONFIG_H -DQUICK -DBUILTIN_ELF \
		-DELFCORE -DVERSION='"${VERSION}"'
CPPFLAGS+=	-I${.CURDIR}/../include -I${DIST}/src

CPPFLAGS+=  	-D__SELECT_DECLARED

.PATH:		${DIST}/src ${DIST}/doc

#DPLIBS+=     lzma    ${NETBSDSRCDIR}/contrib/xz/lib
DPLIBS+=     bz2     ${NETBSDSRCDIR}/usr.lib/libbz2
DPLIBS+=     z       ${NETBSDSRCDIR}/usr.lib/libz

