if HAVE_SIDPLAY

lib_LTLIBRARIES = libsid.la

endif

###
### Source targets
###
EXTRA_DIST = xmms-sid-logo.xpm

# Generals
AM_CFLAGS   = -D_REENTRANT $(PLUGIN_CFLAGS) @SIDPLAY1_INCLUDES@ @SIDPLAY2_INCLUDES@ @BUILDERS_INCLUDES@
AM_CXXFLAGS = $(AM_CFLAGS)

# Generated libraries

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

libsid_la_LDFLAGS = $(PLUGIN_LDFLAGS) @BUILDERS_LDFLAGS@
libsid_la_LIBADD  = @SIDPLAY1_LDADD@ @SIDPLAY2_LDADD@ @RESID_LDADD@ @HARDSID_LDADD@
libsid_la_SOURCES =	\
	xs_init.c			\
	xs_about.c			\
	xs_support.c	xs_support.h	\
	xs_config.c	xs_config.h	\
	xs_md5.c	xs_md5.h	\
	xs_length.c	xs_length.h	\
	xs_genui.c	xs_genui.h	\
	xs_glade.c	xs_glade.h	\
	xs_interface.c	xs_interface.h	\
	xs_stil.c	xs_stil.h	\
	xs_title.c	xs_title.h	\
	xs_fileinfo.c	xs_fileinfo.h	\
	xs_filter.c	xs_filter.h	\
	xs_sidplay.h			\
	xs_sidplay1.cc	xs_sidplay1.h	\
	xs_sidplay2.cc	xs_sidplay2.h	\
	xmms-sid.c	xmms-sid.h

INCLUDES = $(GTK_CFLAGS) -I$(top_builddir)/intl -I$(top_srcdir)

