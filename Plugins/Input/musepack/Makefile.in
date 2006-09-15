include ../../../mk/rules.mk
include ../../../mk/init.mk

OBJECTIVE_LIBS = libmpc$(SHARED_SUFFIX)

LIBDIR = $(plugindir)/$(INPUT_PLUGIN_DIR)

SOURCES = equalizer.cxx libmpc.cxx

LIBADD = -lmpcdec $(TAGLIB_LIBS) $(GTK_LIBS)

OBJECTS = ${SOURCES:.cxx=.o}

CXXFLAGS += $(PICFLAGS) $(GTK_CFLAGS) $(TAGLIB_CFLAGS) -I../../../intl -I../../..

include ../../../mk/objective.mk
