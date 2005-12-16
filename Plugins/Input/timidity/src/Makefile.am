lib_LTLIBRARIES = libtimidity.la

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

libtimidity_la_LDFLAGS = $(PLUGIN_LDFLAGS) ../libtimidity/libtimidity.la

libtimidity_la_SOURCES = \
	callbacks.c \
	callbacks.h \
	interface.c \
	interface.h \
	xmms-timidity.c \
	xmms-timidity.h

INCLUDES = $(GTK_CFLAGS) -I$(top_builddir)/intl -I$(top_srcdir) -I../libtimidity
