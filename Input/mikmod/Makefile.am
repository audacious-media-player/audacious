if HAVE_MIKMOD

lib_LTLIBRARIES = libmikmod.la

endif

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

INCLUDES = @GTK_CFLAGS@ @LIBMIKMOD_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)
libmikmod_la_LDFLAGS = @PLUGIN_LDFLAGS@
libmikmod_la_LIBADD = @GTK_LIBS@ @PTHREAD_LIBS@ \
@LIBMIKMOD_LIBS@ @LIBMIKMOD_LDADD@ 

libmikmod_la_SOURCES = drv_xmms.c mikmod-plugin.h plugin.c mikmod.xpm

