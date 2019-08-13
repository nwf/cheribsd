# $FreeBSD$

.include <src.opts.mk>

NEED_CHERI=pure
LIBADD+=pthread

PROG=  cheri_bench_malloc
MAN=
SRCS=  main.c \
       capdirty.c \
       malloc_comb.c

.include <bsd.prog.mk>
