include ../../../mk/rules.mk
include ../../../mk/objective.mk

OBJECTIVE_LIBS = libm3u.so

LIBDIR = $(plugindir)/$(CONTAINER_PLUGIN_DIR)

SOURCES = m3u.c

OBJECTS = ${SOURCES:.c=.o}

CFLAGS += -fPIC -DPIC $(GTK_CFLAGS) $(ARCH_DEFINES) -I../../../intl -I../../..

CXXFLAGS = $(CFLAGS)

LIBADD = $(GTK_LIBS)
