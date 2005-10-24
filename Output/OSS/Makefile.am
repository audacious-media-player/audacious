osssources = OSS.c OSS.h \
mixer.c \
about.c \
configure.c \
audio.c \
init.c \
convert.c \
soundcard.h

if HAVE_OSS

ossltlibs = libOSS.la
ossbuildsources = $(osssources)


else

ossltlibs =
ossbuildsources =

endif

EXTRA_DIST = $(osssources)

lib_LTLIBRARIES = $(ossltlibs)

libdir = $(plugindir)/$(OUTPUT_PLUGIN_DIR)

libOSS_la_LDFLAGS = @PLUGIN_LDFLAGS@
libOSS_la_LIBADD = @GTK_LIBS@
libOSS_la_SOURCES= $(ossbuildsources)
INCLUDES = $(GTK_CFLAGS) -I$(top_builddir)/intl -I$(top_srcdir)
