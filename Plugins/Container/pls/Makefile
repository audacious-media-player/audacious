include ../../../mk/rules.mk
include ../../../mk/init.mk

OBJECTIVE_LIBS = libpls.so

LIBDIR = $(plugindir)/$(CONTAINER_PLUGIN_DIR)

SOURCES = pls.c

OBJECTS = ${SOURCES:.c=.o}

CFLAGS += -fPIC -DPIC $(GTK_CFLAGS) $(ARCH_DEFINES) -I../../../intl -I../../..

CXXFLAGS = $(CFLAGS)

LIBADD = $(GTK_LIBS)

include ../../../mk/objective.mk
