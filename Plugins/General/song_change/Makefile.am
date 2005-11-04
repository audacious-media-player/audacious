lib_LTLIBRARIES = libsong_change.la

libdir = $(plugindir)/$(GENERAL_PLUGIN_DIR)

libsong_change_la_LDFLAGS = @PLUGIN_LDFLAGS@
libsong_change_la_LIBADD = @GTK_LIBS@ @PTHREAD_LIBS@

libsong_change_la_SOURCES = song_change.c

INCLUDES = @GTK_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)
