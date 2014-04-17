include ../../extra.mk

PROG = audacious${PROG_SUFFIX}

SRCS = main.c \
       signals.c

ifeq ($(HAVE_MSWINDOWS),yes)
SRCS += audacious.rc
endif

ifeq ($(USE_DBUS),yes)
SRCS += dbus-server.c
EXT_DEPS += ../dbus/aud-dbus.a
endif

include ../../buildsys.mk

ifeq ($(USE_DBUS),yes)
CPPFLAGS := -I../dbus ${CPPFLAGS} ${GIO_CFLAGS}
LIBS := ../dbus/aud-dbus.a ${LIBS} ${GIO_LIBS}
endif

CPPFLAGS := -I.. -I../.. \
            ${CPPFLAGS} \
            ${GLIB_CFLAGS}

CPPFLAGS += -D_AUDACIOUS_CORE

LIBS := -L../libaudcore -laudcore \
        ${LIBS} -lm \
        ${LIBINTL} \
        ${GLIB_LIBS}

