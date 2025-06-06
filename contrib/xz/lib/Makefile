# $NetBSD: Makefile,v 1.10 2018/09/25 05:42:08 joerg Exp $

.include <bsd.init.mk>

LIB=		lzma
USE_SHLIBDIR=	yes
NOLINT=		yes

.include "Makefile.compat"

.PATH:	${XZSRCDIR}/src/liblzma/api ${XZSRCDIR}/src/liblzma/api/lzma
INCS+=	base.h bcj.h block.h check.h container.h delta.h \
	filter.h hardware.h index.h index_hash.h lzma.h lzma12.h \
	stream_flags.h version.h vli.h

INCSDIR_lzma.h=	/usr/include
INCSDIR=	/usr/include/lzma

.if ${MKSANITIZER} != "yes"
LDFLAGS+=	-Wl,-z,defs
.endif

CPPFLAGS+=	-I${XZSRCDIR}/src/common
CPPFLAGS+=	-I${XZSRCDIR}/src/liblzma/check
CPPFLAGS+=	-I${XZSRCDIR}/src/liblzma/common
CPPFLAGS+=	-I${XZSRCDIR}/src/liblzma/delta
CPPFLAGS+=	-I${XZSRCDIR}/src/liblzma/lz
CPPFLAGS+=	-I${XZSRCDIR}/src/liblzma/lzma
CPPFLAGS+=	-I${XZSRCDIR}/src/liblzma/rangecoder
CPPFLAGS+=	-I${XZSRCDIR}/src/liblzma/simple
CPPFLAGS+=	-DTUKLIB_SYMBOL_PREFIX=lzma_
CPPFLAGS+=	-DNETBSD_NATIVE_SHA256

.PATH:	${XZSRCDIR}/src/common
SRCS+=	tuklib_physmem.c tuklib_cpucores.c

.PATH:	${XZSRCDIR}/src/liblzma/check
SRCS+=	check.c crc32_table.c crc64_table.c
SRCS+=	crc32_fast.c crc64_fast.c

.if defined(HOSTLIB)
SRCS+=	sha256.c
.endif

.PATH:	${XZSRCDIR}/src/liblzma/common
SRCS+=	common.c block_util.c easy_preset.c filter_common.c \
	hardware_physmem.c hardware_cputhreads.c index.c \
	stream_flags_common.c vli_size.c \
	alone_encoder.c block_buffer_encoder.c block_encoder.c \
	block_header_encoder.c easy_buffer_encoder.c easy_encoder.c \
	easy_encoder_memusage.c filter_buffer_encoder.c \
	filter_encoder.c filter_flags_encoder.c index_encoder.c \
	stream_buffer_encoder.c stream_encoder.c \
	stream_flags_encoder.c vli_encoder.c outqueue.c \
	alone_decoder.c auto_decoder.c block_buffer_decoder.c \
	block_decoder.c block_header_decoder.c easy_decoder_memusage.c \
	filter_buffer_decoder.c filter_decoder.c filter_flags_decoder.c \
	index_decoder.c index_hash.c stream_buffer_decoder.c \
	stream_decoder.c stream_flags_decoder.c vli_decoder.c

.if !defined(HOSTLIB)
SRCS+=	 stream_encoder_mt.c
.endif

.PATH:	${XZSRCDIR}/src/liblzma/delta
SRCS+=	delta_common.c delta_encoder.c delta_decoder.c

.PATH:	${XZSRCDIR}/src/liblzma/lz
SRCS+=	lz_decoder.c lz_encoder.c lz_encoder_mf.c

.PATH:	${XZSRCDIR}/src/liblzma/lzma
SRCS+=	lzma_encoder.c lzma_encoder_presets.c \
	lzma_encoder_optimum_fast.c lzma_encoder_optimum_normal.c \
	lzma_decoder.c lzma2_encoder.c lzma2_decoder.c \
	fastpos_table.c

.PATH:	${XZSRCDIR}/src/liblzma/rangecoder
SRCS+=	price_table.c

.PATH:	${XZSRCDIR}/src/liblzma/simple
SRCS+=	simple_coder.c simple_encoder.c simple_decoder.c \
	arm.c armthumb.c ia64.c powerpc.c sparc.c x86.c

liblzma.pc:	${XZSRCDIR}/src/liblzma/liblzma.pc.in
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	${TOOL_SED} \
	    -e 's,@prefix@,/usr,' \
	    -e 's,@exec_prefix@,/usr,' \
	    -e 's,@libdir@,/usr/lib,' \
	    -e 's,@includedir@,/usr/include,' \
	    -e 's,@PACKAGE_HOMEPAGE@,${XZHOMEPAGE:Q},' \
	    -e 's,@PACKAGE_VERSION@,${XZVERSION:Q},' \
	    -e 's,@PTHREAD_CFLAGS@ @PTHREAD_LIBS@,-lpthread,' \
	    < ${XZSRCDIR}/src/liblzma/liblzma.pc.in \
	    > ${.TARGET}.tmp && \
	mv -f ${.TARGET}.tmp ${.TARGET}

.if ${MACHINE_ARCH} == "i386"
COPTS += ${${ACTIVE_CC} == "gcc":? -Wno-error=psabi :}
.endif

CLEANFILES+=	liblzma.pc

.if defined(HOSTLIB)
HOST_CPPFLAGS+=	${CPPFLAGS:N-Wp,-iremap,*}
.include <bsd.hostlib.mk>
.else
LIBDPLIBS+=	pthread ${NETBSDSRCDIR}/usr.lib/libpthread
.include <bsd.lib.mk>
.endif

