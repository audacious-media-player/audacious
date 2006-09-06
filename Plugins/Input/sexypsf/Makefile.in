include ../../../mk/rules.mk
include ../../../mk/init.mk

OBJECTIVE_LIBS = libsexypsf$(SHARED_SUFFIX)

LIBDIR = $(plugindir)/$(INPUT_PLUGIN_DIR)

SOURCES = \
	PsxBios.c \
	PsxCounters.c \
	PsxDma.c \
	Spu.c \
	PsxMem.c \
	PsxHw.c \
	Misc.c \
	R3000A.c \
	PsxInterpreter.c \
	PsxHLE.c \
	spu/spu.c \
	xmms.c

LIBADD = -lz

OBJECTS = ${SOURCES:.c=.o}

CFLAGS += -Wno-nonnull $(PICFLAGS) $(GTK_CFLAGS) -I../../../intl -I../../.. -Ispu/ -I.

include ../../../mk/objective.mk

clean-posthook:
	@rm -f spu/*.o
