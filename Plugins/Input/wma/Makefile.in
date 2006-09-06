include ../../../mk/rules.mk
include ../../../mk/init.mk

OBJECTIVE_LIBS = libwma$(SHARED_SUFFIX)

SUBDIRS = libffwma

LIBDIR = $(plugindir)/$(INPUT_PLUGIN_DIR)

LIBADD = ./libffwma/libffwma.a

SOURCES = wma.c

OBJECTS = ${SOURCES:.c=.o}

CFLAGS += $(PICFLAGS) $(GTK_CFLAGS) -I../../../intl -I../../.. -I./libffwma -std=c99

include ../../../mk/objective.mk
