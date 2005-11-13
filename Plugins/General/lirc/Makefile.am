if HAVE_LIRC

lib_LTLIBRARIES = liblirc.la

endif

libdir = $(plugindir)/$(GENERAL_PLUGIN_DIR)

liblirc_la_LDFLAGS = -llirc_client $(PLUGIN_LDFLAGS)
liblirc_la_SOURCES = about.c lirc.c lirc.h lirc_image.h

EXTRA_DIST = lirc_mini.xpm
INCLUDES = $(GTK_CFLAGS) -I$(top_builddir)/intl -I$(top_srcdir)

