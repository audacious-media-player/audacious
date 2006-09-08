include ../../../mk/rules.mk
include ../../../mk/init.mk

OBJECTIVE_LIBS = libwa2gui$(SHARED_SUFFIX)

SUBDIRS = widgets

noinst_HEADERS = coreaudio.h

LIBDIR = $(plugindir)/$(USERINTERFACE_PLUGIN_DIR)

LIBADD = $(GTK_LIBS)
SOURCES = mainwin.c equalizer.c playlist.c wa2gui.c

OBJECTS = ${SOURCES:.c=.o}

CFLAGS += $(PICFLAGS) $(GTK_CFLAGS) -I../../../intl -I../../..

include ../../../mk/objective.mk
