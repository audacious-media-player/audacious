include ../../../mk/rules.mk
include ../../../mk/objective.mk

OBJECTIVE_LIBS = libxspf.so

LIBDIR = $(plugindir)/$(CONTAINER_PLUGIN_DIR)

SOURCES = xspf.c

OBJECTS = ${SOURCES:.c=.o}

CFLAGS += -fPIC -DPIC $(GTK_CFLAGS) $(ARCH_DEFINES) $(XML_CPPFLAGS) -I../../../intl -I../../..

CXXFLAGS = $(CFLAGS)

LIBADD = $(GTK_LIBS) $(XML_LIBS)
