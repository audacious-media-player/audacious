if HAVE_MODPLUG

lib_LTLIBRARIES        = libmodplug.la
SUBDIRS = archive gui

endif

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

INCLUDES = @GTK_CFLAGS@ -I.. -I$(top_srcdir) -I$(top_builddir)/intl
libmodplug_la_LDFLAGS  = $(PLUGIN_LDFLAGS) $(GTK_LIBS) -lstdc++ -lmodplug ./archive/libarchive.la ./gui/libgui.la
libmodplug_la_SOURCES  = plugin.cpp modplugbmp.cpp

noinst_HEADERS = modplugbmp.h stddefs.h
