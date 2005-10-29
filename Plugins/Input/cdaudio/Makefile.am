cdaudiosources = cdaudio.c \
configure.c \
cdaudio.h \
cdinfo.c \
cdinfo.h \
cddb.c \
cddb.h \
http.c \
http.h

if HAVE_CDROM

cdaudioltlibs = libcdaudio.la
cdaudiobuildsources = $(cdaudiosources)

else

cdaudioltlibs =
cdaudiobuildsources =

endif

EXTRA_DIST = $(cdaudiosources)

lib_LTLIBRARIES = $(cdaudioltlibs)

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

INCLUDES = @GTK_CFLAGS@ @GCONF_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)
libcdaudio_la_LDFLAGS = @PLUGIN_LDFLAGS@
libcdaudio_la_LIBADD = @GTK_LIBS@
libcdaudio_la_SOURCES = $(cdaudiobuildsources)

