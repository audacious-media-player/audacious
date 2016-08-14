include ../../extra.mk

PROG = audacious${PROG_SUFFIX}

SRCS = main.cc \
       signals.cc \
       util.cc

ifeq ($(HAVE_MSWINDOWS),yes)
SRCS += audacious.rc
endif

ifeq ($(USE_DBUS),yes)
SRCS += dbus-server.cc
EXT_DEPS += ../dbus/aud-dbus.a
endif

include ../../buildsys.mk

ifeq ($(USE_DBUS),yes)
CPPFLAGS := -I../dbus ${CPPFLAGS} ${GIO_CFLAGS}
LIBS := ../dbus/aud-dbus.a ${LIBS} ${GIO_LIBS}
endif

LD = ${CXX}

CPPFLAGS := -I.. -I../.. \
            ${CPPFLAGS} \
            ${GLIB_CFLAGS}

LIBS := -L../libaudcore -laudcore \
        ${LIBS} -lm \
        ${LIBINTL} \
        ${GLIB_LIBS}

ifeq ($(HAVE_MSWINDOWS),yes)
LDFLAGS := ${LDFLAGS} -Wl,-subsystem,windows
endif
