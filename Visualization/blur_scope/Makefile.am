lib_LTLIBRARIES = libbscope.la

libdir = $(plugindir)/$(VISUALIZATION_PLUGIN_DIR)

libbscope_la_LDFLAGS = $(PLUGIN_LDFLAGS)
libbscope_la_LIBADD = $(GTK_LIBS)

COMMON_SRC = blur_scope.c blur_scope.h config.c

if USE_X86ASM

EXTRA_SRC = blur_8.s

endif

EXTRA_DIST = blur_8.s xmms_logo.xpm bscope_logo.xpm

libbscope_la_SOURCES =  $(COMMON_SRC) $(EXTRA_SRC)

INCLUDES = $(GTK_CFLAGS) $(ARCH_DEFINES) -I$(top_builddir)/intl -I$(top_srcdir)
