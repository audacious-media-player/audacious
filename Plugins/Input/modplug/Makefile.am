if HAVE_MODPLUG

lib_LTLIBRARIES        = libmodplug.la
SUBDIRS = archive gui

endif

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

INCLUDES = @GTK_CFLAGS@ -I.. -I$(top_srcdir) -I$(top_builddir)/intl
libmodplug_la_LDFLAGS  = -module -avoid-version -lstdc++
libmodplug_la_SOURCES  = plugin.cpp modplugbmp.cpp
libmodplug_la_LIBADD   = ./archive/libarchive.la ./gui/libgui.la -lmodplug $(GTK_LIBS)

noinst_HEADERS = modplugbmp.h plugin.h stddefs.h
