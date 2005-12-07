if ENABLE_CROSSFADE
lib_LTLIBRARIES = libcrossfade.la
endif

gtk_sources = \
	interface-2.0.c \
	interface-2.0.h \
	support-2.0.c \
	support-2.0.h \
	callbacks.c \
	callbacks.h

if HAVE_LIBSAMPLERATE
samplerate_libs = -lsamplerate
endif

libdir = $(plugindir)/$(OUTPUT_PLUGIN_DIR)

libcrossfade = @PLUGIN_LDFLAGS@
libcrossfade_la_LIBADD  = $(samplerate_libs)
libcrossfade_la_SOURCES = \
	crossfade.c \
	crossfade.h \
	configure.c \
	configure.h \
	convert.c \
	convert.h \
	debug.c \
	debug.h \
	effect.c \
	effect.h \
	format.c \
	format.h \
	monitor.c \
	monitor.h \
	rate.c \
	rate.h \
	timing.c \
	timing.h \
	volume.c \
	volume.h \
	$(gtk_sources)

INCLUDES = $(GTK_CFLAGS) -I$(top_builddir)/intl -I$(top_srcdir)
