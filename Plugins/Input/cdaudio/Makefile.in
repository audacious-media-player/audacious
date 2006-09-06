include ../../../mk/rules.mk
include ../../../mk/init.mk

OBJECTIVE_LIBS = libcdaudio$(SHARED_SUFFIX)

LIBDIR = $(plugindir)/$(INPUT_PLUGIN_DIR)

SOURCES = cdaudio.c cddb.c cdinfo.c configure.c http.c 

OBJECTS = ${SOURCES:.c=.o}

CFLAGS += $(PICFLAGS) @GTK_CFLAGS@ -I../../../intl -I../../.. -I../..

include ../../../mk/objective.mk
