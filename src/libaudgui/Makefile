LIB = ${LIB_PREFIX}audgui${LIB_SUFFIX}
LIB_MAJOR = 1
LIB_MINOR = 0

SRCS = ui_gtk.c \
       ui_fileopener.c

INCLUDES = ui_gtk.h \
           ui_fileopener.h

include ../../buildsys.mk
include ../../extra.mk

includesubdir = libaudgui

CPPFLAGS += ${LIB_CPPFLAGS} ${CFLAGS} -D_AUDACIOUS_CORE -I.. -I../..
CFLAGS += ${LIB_CFLAGS} ${GLIB_CFLAGS} ${GTK_CFLAGS} ${DBUS_CFLAGS} ${MOWGLI_CFLAGS} $(AUDACIOUS_DEFINES)
LDFLAGS += $(AUDLDFLAGS)
LIBS += ${GLIB_LIBS} ${MOWGLI_LIBS} ${GTK_LIBS}
