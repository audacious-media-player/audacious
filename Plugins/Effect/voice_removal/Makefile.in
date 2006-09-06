include ../../../mk/rules.mk
include ../../../mk/init.mk

OBJECTIVE_LIBS = libvoice_removal$(SHARED_SUFFIX)

LIBDIR = $(plugindir)/$(EFFECT_PLUGIN_DIR)

LIBADD = $(GTK_LIBS)

SOURCES = voice_removal.c

CFLAGS += $(PICFLAGS) $(GTK_CFLAGS) -I../../../intl -I../../..

OBJECTS = ${SOURCES:.c=.o}

include ../../../mk/objective.mk
