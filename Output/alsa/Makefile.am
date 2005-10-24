alsasources = \
alsa.c alsa.h \
about.c \
audio.c \
configure.c \
init.c

if HAVE_ALSA

alsaltlibs = libALSA.la
alsabuildsources = $(alsasources)

else

alsaltlibs =
alsabuildsources = 

endif

EXTRA_DIST = $(alsasources)

lib_LTLIBRARIES = $(alsaltlibs)

libdir = $(plugindir)/$(OUTPUT_PLUGIN_DIR)

libALSA_la_LDFLAGS = @PLUGIN_LDFLAGS@
libALSA_la_LIBADD = @GTK_LIBS@ @ALSA_LIBS@
libALSA_la_SOURCES= $(alsabuildsources)
INCLUDES = $(GTK_CFLAGS) $(ALSA_CFLAGS) -I$(top_builddir)/intl -I$(top_srcdir)
