SUBDIRS = libffwma

lib_LTLIBRARIES = libwma.la

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

libwma_la_LDFLAGS = $(PLUGIN_LDFLAGS) ./libffwma/libffwma.la

libwma_la_SOURCES = wma.c iir.c

INCLUDES = $(GTK_CFLAGS) -I$(top_builddir)/intl -I$(top_srcdir) -I./libffwma
