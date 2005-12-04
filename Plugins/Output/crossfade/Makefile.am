lib_LTLIBRARIES = libcrossfade.la

gtk_sources = \
	interface-2.0.c \
	interface-2.0.h \
	support-2.0.c \
	support-2.0.h \
	callbacks.c \
	callbacks.h
interface-2.0.c:
	glade-2 -w interface-2.0.glade

if HAVE_LIBFFTW_COND
fftw_sources = fft.c fft.h
fftw_libs = -lfftw -lrfftw
endif

if HAVE_LIBSAMPLERATE_COND
samplerate_libs = -lsamplerate
endif

libdir = $(plugindir)/$(OUTPUT_PLUGIN_DIR)

libcrossfade = @PLUGIN_LDFLAGS@
libcrossfade_la_LIBADD  = $(fftw_libs) $(samplerate_libs)
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
	$(gtk_sources) \
	$(fftw_sources)

