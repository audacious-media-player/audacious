include ../../../mk/rules.mk
include ../../../mk/init.mk

OBJECTIVE_LIBS = libcoreaudio$(SHARED_SUFFIX)

noinst_HEADERS = coreaudio.h

LIBDIR = $(plugindir)/$(OUTPUT_PLUGIN_DIR)

LIBADD = $(GTK_LIBS) -Wl,-framework,CoreAudio
SOURCES = coreaudio.c mixer.c about.c configure.c audio.c init.c convert.c dbconvert.c

OBJECTS = ${SOURCES:.c=.o}

CFLAGS += $(PICFLAGS) $(GTK_CFLAGS) -I../../../intl -I../../..

include ../../../mk/objective.mk
