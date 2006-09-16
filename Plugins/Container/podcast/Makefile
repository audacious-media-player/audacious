include ../../../mk/rules.mk
include ../../../mk/init.mk

OBJECTIVE_LIBS = libpodcast$(SHARED_SUFFIX)

LIBDIR = $(plugindir)/$(CONTAINER_PLUGIN_DIR)

SOURCES = podcast.c

OBJECTS = ${SOURCES:.c=.o}

CFLAGS += $(PICFLAGS) $(GTK_CFLAGS) $(ARCH_DEFINES) $(XML_CPPFLAGS) -I../../../intl -I../../..

CXXFLAGS = $(CFLAGS)

LIBADD = $(GTK_LIBS) $(XML_LIBS)

include ../../../mk/objective.mk
