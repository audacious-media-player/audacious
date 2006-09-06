include ../../../mk/rules.mk
include ../../../mk/init.mk

OBJECTIVE_LIBS = libwav$(SHARED_SUFFIX)

LIBDIR = $(plugindir)/$(INPUT_PLUGIN_DIR)

SOURCES = wav$(WAV_SNDFILE).c

LIBADD = -L/usr/local/lib $(SNDFILE_LIBS)

CFLAGS += $(PICFLAGS) $(GTK_CFLAGS) -I../../../intl -I../../..

OBJECTS = ${SOURCES:.c=.o}

include ../../../mk/objective.mk
