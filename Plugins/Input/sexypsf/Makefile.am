lib_LTLIBRARIES = libsexypsf.la

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

libsexypsf_la_LDFLAGS = $(PLUGIN_LDFLAGS)

libsexypsf_la_SOURCES = \
	PsxCommon.h types.h \
	PsxBios.c PsxBios.h \
	PsxCounters.c PsxCounters.h \
	PsxDma.c PsxDma.h \
	Spu.c Spu.h \
	PsxMem.c PsxMem.h \
	PsxHw.c PsxHw.h \
	Misc.c Misc.h driver.h \
	R3000A.c R3000A.h \
	PsxInterpreter.c \
	PsxHLE.c PsxHLE.h \
	spu/spu.c \
	xmms.c

libsexypsf_la_LIBADD = -lz

INCLUDES = $(GTK_CFLAGS) -I$(top_builddir)/intl -I$(top_srcdir)
