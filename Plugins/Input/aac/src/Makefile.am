lib_LTLIBRARIES = libaac.la

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

libaac_la_LDFLAGS = $(PLUGIN_LDFLAGS) ../libmp4v2/libmp4v2.la ../libfaad2/libfaad.la -lstdc++

libaac_la_SOURCES = libmp4.c mp4_utils.c aac_utils.c

INCLUDES = $(GTK_CFLAGS) -I$(top_builddir)/intl -I$(top_srcdir) -I../include -I../libmp4v2 -I../libfaad
