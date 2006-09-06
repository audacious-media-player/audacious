include ../../../mk/rules.mk
include ../../../mk/init.mk

CFLAGS += $(PICFLAGS) -Ipn/ -I../../.. $(ARCH_DEFINES) $(GTK_CFLAGS) $(SDL_CFLAGS) $(XML_CFLAGS) $(XML_CPPFLAGS)

SUBDIRS = pn

LIBDIR = $(plugindir)/$(VISUALIZATION_PLUGIN_DIR)

LIBADD += $(GTK_LIBS) $(SDL_LIBS) $(XML_LIBS) pn/libparanormal.a

OBJECTIVE_LIBS = libparanormal$(SHARED_SUFFIX)

SOURCES = client.c

OBJECTS = ${SOURCES:.c=.o}

include ../../../mk/objective.mk
