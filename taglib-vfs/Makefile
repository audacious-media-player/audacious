include ../mk/rules.mk
include ../mk/init.mk

beepincludedir = $(includedir)/audacious

OBJECTIVE_LIBS = libtagvfs.so

LDFLAGS += $(AUDLDFLAGS)

CXXFLAGS += $(LIBLDFLAGS) \
	$(GTK_CFLAGS)      \
	$(LIBGLADE_CFLAGS) \
	$(BEEP_DEFINES)    \
	$(ARCH_DEFINES)    \
	-D_AUDACIOUS_CORE \
	-I..    \
	-I../intl \
	-I/usr/include/taglib

HEADERS = \
	tvfsfile.h

SOURCES = tvfsfile.cxx

OBJECTS = ${SOURCES:.cxx=.o}

beepinclude_HEADERS = plugin.h output.h input.h

desktop_DATA = audacious.desktop
desktopdir = $(datadir)/applications

include ../mk/objective.mk
