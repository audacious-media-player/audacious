lib_LTLIBRARIES = libwav.la

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

if HAVE_LIBSNDFILE

libwav_la_SOURCES =	\
	wav-sndfile.c	\
	wav-sndfile.h

libwav_la_LDFLAGS = $(PLUGIN_LDFLAGS) -lsndfile

else

libwav_la_SOURCES = 	\
	wav.c \
	wav.h

libwav_la_LDFLAGS = $(PLUGIN_LDFLAGS)

endif

INCLUDES = $(GTK_CFLAGS) -I$(top_builddir)/intl -I$(top_srcdir)
