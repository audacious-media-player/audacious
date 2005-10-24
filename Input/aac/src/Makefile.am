lib_LTLIBRARIES = libaac.la

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

libaac_la_LDFLAGS = $(PLUGIN_LDFLAGS)

libaac_la_SOURCES = aac_utils.c libmp4.c mp4_utils.c

INCLUDES = $(GTK_CFLAGS) -I$(top_builddir)/intl -I$(top_srcdir) -I../libmp4v2 -I../libfaad
