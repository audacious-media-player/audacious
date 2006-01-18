ACLOCAL_AMFLAGS = -I ../m4
AUTOMAKE_OPTIONS = foreign

beepincludedir = $(includedir)/audacious

lib_LTLIBRARIES = libaudacious.la

libaudacious_la_LDFLAGS = -export-dynamic -version-info $(LIBBEEP_MAJOR_VERSION):$(LIBBEEP_MINOR_VERSION):$(LIBBEEP_MICRO_VERSION)

libaudacious_la_LIBADD = \
	$(GTK_LIBS)      \
	$(GCONF_LIBS)    \
	$(GNOMEVFS_LIBS)

INCLUDES = \
	$(GTK_CFLAGS)        \
	$(GCONF_CFLAGS)      \
	$(GNOMEVFS_CFLAGS)   \
	-I$(top_srcdir)      \
	-I$(top_srcdir)/intl

if HAVE_GCONF
    CONF_SRC = configdb_gconf.c
else
    CONF_SRC = configdb_rcfile.c
endif

if HAVE_GNOME_VFS
    VFS_SRC = vfs_gnome.c
else
    VFS_SRC = vfs_stdio.c
endif

libaudacious_la_SOURCES = \
	$(CONF_SRC) configdb.h \
	rcfile.c rcfile.h \
	$(VFS_SRC) vfs.h \
	configfile.c \
	beepctrl.c beepctrl.h \
	dirbrowser.c dirbrowser.h \
	util.c util.h \
	formatter.c formatter.h \
	titlestring.c titlestring.h \
	xentry.c xentry.h \
	xconvert.c xconvert.h \
	xml_document.c xml_document.h

beepinclude_HEADERS = \
	vfs.h rcfile.h configdb.h \
	configfile.h \
	beepctrl.h dirbrowser.h util.h \
	formatter.h titlestring.h xml_document.h
