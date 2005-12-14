ACLOCAL_AMFLAGS = -I ../m4
AUTOMAKE_OPTIONS = foreign

beepincludedir = $(includedir)/audacious

lib_LTLIBRARIES = libaudcore.la

libaudcore_la_LDFLAGS = -export-dynamic -version-info $(LIBBEEP_MAJOR_VERSION):$(LIBBEEP_MINOR_VERSION):$(LIBBEEP_MICRO_VERSION)

libaudcore_la_LIBADD = \
	$(GTK_LIBS)      \
	$(GCONF_LIBS)    \
	$(GNOMEVFS_LIBS)

INCLUDES = \
	$(GTK_CFLAGS)        \
	$(GCONF_CFLAGS)      \
	$(GNOMEVFS_CFLAGS)   \
	-I$(top_srcdir)      \
	-I$(top_srcdir)/intl	\
	-I$(top_srcdir)/audacious

libaudcore_la_SOURCES = \
	playback.c playback.h
