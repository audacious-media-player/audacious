include ../../../mk/rules.mk
include ../../../mk/objective.mk

OBJECTIVE_LIBS = libnotify.so

LIBDIR = $(plugindir)/$(GENERAL_PLUGIN_DIR)

SOURCES = notify.c

OBJECTS = ${SOURCES:.c=.o}

CFLAGS += -fPIC -DPIC $(GTK_CFLAGS) $(LIBNOTIFY_CFLAGS) $(ARCH_DEFINES) -I../../../intl -I../../.. -DDATA_DIR=\"$(datadir)\"

CXXFLAGS = $(CFLAGS)

LIBADD = $(GTK_LIBS) $(LIBNOTIFY_LIBS)
