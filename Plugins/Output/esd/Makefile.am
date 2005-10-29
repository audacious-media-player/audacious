if HAVE_ESD

esdltlibs = libesdout.la
esdsources = esd.c esdout.h \
mixer.c \
about.c \
configure.c \
audio.c \
init.c

else

esdltlibs =
esdsources =

endif

EXTRA_DIST = esd.c esdout.h mixer.c about.c configure.c audio.c init.c

libdir = $(plugindir)/$(OUTPUT_PLUGIN_DIR)
lib_LTLIBRARIES = $(esdltlibs)

INCLUDES = $(GTK_CFLAGS) $(ESD_CFLAGS) -I$(top_builddir)/intl -I$(top_srcdir)
libesdout_la_LDFLAGS = $(PLUGIN_LDFLAGS)
libesdout_la_LIBADD = $(GTK_LIBS) $(ESD_LIBS)

libesdout_la_SOURCES= $(esdsources)
