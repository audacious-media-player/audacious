include ../../../mk/rules.mk
include ../../../mk/init.mk

OBJECTIVE_LIBS = libpls$(SHARED_SUFFIX)

LIBDIR = $(plugindir)/$(CONTAINER_PLUGIN_DIR)

SOURCES = pls.c

OBJECTS = ${SOURCES:.c=.o}

CFLAGS += $(PICFLAGS) $(GTK_CFLAGS) $(ARCH_DEFINES) -I../../../intl -I../../..

CXXFLAGS = $(CFLAGS)

LIBADD = $(GTK_LIBS)

include ../../../mk/objective.mk
