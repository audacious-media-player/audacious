if HAVE_ADPLUG

SUBDIRS = core
lib_LTLIBRARIES = libadplug.la

endif

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

libadplug_la_LDFLAGS = -lstdc++ $(BINIO_LIBS) $(PLUGIN_LDFLAGS) ./core/libadplugcore.la

libadplug_la_SOURCES = adplug-xmms.cc

INCLUDES = $(GTK_CFLAGS) $(BINIO_CFLAGS) -I$(top_builddir)/intl -I$(top_srcdir) -I./core
