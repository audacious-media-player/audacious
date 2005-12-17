if ENABLE_MUSEPACK

lib_LTLIBRARIES = libmpc.la

endif

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

libmpc_la_LIBADD = @GTK_LIBS@ @TAGLIB_LIBS@

libmpc_la_SOURCES = equalizer.cpp libmpc.cpp
libmpc_la_LDFLAGS = -lmpcdec -avoid-version
noinst_HEADERS = equalizer.h libmpc.h

INCLUDES = $(GTK_CFLAGS) $(TAGLIB_CFLAGS) -I$(top_builddir)/intl -I$(top_srcdir)
