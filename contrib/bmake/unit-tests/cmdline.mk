# $NetBSD: cmdline.mk,v 1.1 2020/07/28 22:44:44 rillig Exp $
#
# Tests for command line parsing and related special variables.

RUN?=		@set -eu;
TMPBASE?=	/tmp
SUB1=		a7b41170-53f8-4cc2-bc5c-e4c3dd93ec45	# just a random UUID
SUB2=		6a8899d2-d227-4b55-9b6b-f3c8eeb83fd5	# just a random UUID
MAKE_CMD=	env TMPBASE=${TMPBASE}/${SUB1} ${.MAKE} -f ${MAKEFILE} -r
DIR2=		${TMPBASE}/${SUB2}
DIR12=		${TMPBASE}/${SUB1}/${SUB2}

all: prepare-dirs
all: makeobjdir-direct makeobjdir-indirect

prepare-dirs:
	${RUN} rm -rf ${DIR2} ${DIR12}
	${RUN} mkdir -p ${DIR2} ${DIR12}

# The .OBJDIR can be set via the MAKEOBJDIR command line variable.
# It must be a command line variable; an environment variable would not work.
makeobjdir-direct:
	@echo $@:
	${RUN} ${MAKE_CMD} MAKEOBJDIR=${DIR2} show-objdir

# The .OBJDIR can be set via the MAKEOBJDIR command line variable,
# and that variable could even contain the usual modifiers.
# Since the .OBJDIR=MAKEOBJDIR assignment happens very early,
# the SUB2 variable in the modifier is not defined yet and is therefore empty.
# The SUB1 in the resulting path comes from the environment variable TMPBASE,
# see MAKE_CMD.
makeobjdir-indirect:
	@echo $@:
	${RUN} ${MAKE_CMD} MAKEOBJDIR='$${TMPBASE}/$${SUB2}' show-objdir

show-objdir:
	@echo $@: ${.OBJDIR:Q}
