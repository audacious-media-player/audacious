include ../../../mk/rules.mk
include ../../../mk/init.mk

SUBDIRS = core
OBJECTIVE_LIBS = libadplug$(SHARED_SUFFIX)

LIBDIR = $(plugindir)/$(INPUT_PLUGIN_DIR)

LIBADD = ./core/libadplugcore.a $(BINIO_LIBS) -lstdc++

SOURCES = adplug-xmms.cc

OBJECTS = ${SOURCES:.cc=.o}

CFLAGS += $(PICFLAGS) $(GTK_CFLAGS) $(BINIO_CFLAGS) \
	-I../../../intl -I../../.. -I./core
CXXFLAGS += $(PICFLAGS) $(GTK_CFLAGS) $(BINIO_CFLAGS) \
	-I../../../intl -I../../.. -I./core

include ../../../mk/objective.mk
