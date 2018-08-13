# $FreeBSD$

SRCS= \
	shimalloc.c

INCS= 

# I am not sure why this does not work, but it doesn't.
# We'll just sort it out at LD_PRELOAD time
#LIBADD=cheri_caprevoke

CFLAGS+=	-I${.CURDIR}
CFLAGS+=	-I${.CURDIR}/${MACHINE_ARCH}
CFLAGS+=  -mllvm -mxmxgot
CFLAGS+=	-DHAVE_WORKING_TLS

SHLIB=	cheri_shimalloc
SHLIB_MAJOR=	1

.if !defined(NEED_CHERI)
NEED_CHERI=	hybrid
.endif

NO_WERROR=

.include <bsd.lib.mk>
