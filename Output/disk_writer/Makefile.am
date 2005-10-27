lib_LTLIBRARIES = libdisk_writer.la

libdir = $(plugindir)/$(OUTPUT_PLUGIN_DIR)

libdisk_writer_la_LDFLAGS = @PLUGIN_LDFLAGS@
libdisk_writer_la_LIBADD = @GTK_LIBS@
libdisk_writer_la_SOURCES= disk_writer.c

INCLUDES = @GTK_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)
