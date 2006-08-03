include ../../../mk/rules.mk
include ../../../mk/objective.mk

OBJECTIVE_LIBS = libcuesheet.so

LIBDIR = $(plugindir)/$(INPUT_PLUGIN_DIR)

SOURCES = cuesheet.c

OBJECTS = ${SOURCES:.c=.o}

CFLAGS += -fPIC -DPIC $(GTK_CFLAGS) $(ARCH_DEFINES) -I../../../intl -I../../..

CXXFLAGS = $(CFLAGS)

LIBADD = $(GTK_LIBS)
