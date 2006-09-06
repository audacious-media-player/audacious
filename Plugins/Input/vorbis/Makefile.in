include ../../../mk/rules.mk
include ../../../mk/init.mk

OBJECTIVE_LIBS = libvorbis$(SHARED_SUFFIX)

LIBDIR = $(plugindir)/$(INPUT_PLUGIN_DIR)

SOURCES = configure.c  fileinfo.c  http.c  vcedit.c  vorbis.c

OBJECTS = ${SOURCES:.c=.o}

LIBADD = $(OGG_VORBIS_LIBS)

CFLAGS += $(PICFLAGS) $(GTK_CFLAGS) -I../../../intl -I../../.. -I../..

include ../../../mk/objective.mk
