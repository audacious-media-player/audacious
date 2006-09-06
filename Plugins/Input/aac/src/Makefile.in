include ../../../../mk/rules.mk
include ../../../../mk/init.mk

OBJECTIVE_LIBS = libaac$(SHARED_SUFFIX)

CFLAGS += $(PICFLAGS)

# override objective make's LIBDIR
LIBDIR = $(plugindir)/$(INPUT_PLUGIN_DIR)

LDFLAGS += ../libfaad2/libfaad.a ../mp4ff/libmp4ff.a -lstdc++

SOURCES = libmp4.c mp4_utils.c aac_utils.c tagging_mp4.c

OBJECTS = ${SOURCES:.c=.o}

CFLAGS += $(GTK_CFLAGS) -I../../../../intl -I../../../.. -I../include -I../libfaad -I../mp4ff

include ../../../../mk/objective.mk
