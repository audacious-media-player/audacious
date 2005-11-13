lib_LTLIBRARIES = libwav.la

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

libwav_la_LDFLAGS = $(PLUGIN_LDFLAGS)

if HAVE_LIBSNDFILE

libwav_la_SOURCES =	\
	wav-sndfile.c	\
	wav-sndfile.h

libwav_la_LDFLAGS = $(libwav_la_LDFLAGS) -lsndfile

else

libwav_la_SOURCES = 	\
	wav.c \
	wav.h

endif

INCLUDES = $(GTK_CFLAGS) -I$(top_builddir)/intl -I$(top_srcdir)
