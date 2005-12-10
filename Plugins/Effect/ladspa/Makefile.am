lib_LTLIBRARIES = libladspa.la

libdir = $(plugindir)/$(EFFECT_PLUGIN_DIR)

libladspa_la_LDFLAGS = @PLUGIN_LDFLAGS@
libladspa_la_LIBADD = @GTK_LIBS@

libladspa_la_SOURCES = ladspa.c ladspa.h

INCLUDES = @GTK_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)
