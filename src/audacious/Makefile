include ../../extra.mk

PROG = audacious${PROG_SUFFIX}
SRCS = adder.c \
       art.c \
       drct.c \
       effect.c									\
       fft.c									\
       interface.c								\
       main.c									\
       output.c									\
       playback.c								\
       playlist-files.c \
       playlist-new.c \
       playlist-utils.c \
       pluginenum.c								\
       plugin-registry.c \
       plugin-init.c \
       probe.c \
       probe-buffer.c \
       scanner.c \
       signals.c								\
       ui_plugin_menu.c \
       util.c									\
       vis_runner.c \
       visualization.c							\
       ui_albumart.c

ifeq ($(HAVE_MSWINDOWS),yes)
SRCS += audacious.rc
endif

INCLUDES = api.h \
           api-alias-begin.h \
           api-alias-end.h \
           api-define-begin.h \
           api-define-end.h \
           drct.h \
           drct-api.h \
           input.h \
           input-api.h \
           misc.h \
           misc-api.h \
           playlist.h \
           playlist-api.h \
           plugin.h				\
           plugins.h \
           plugins-api.h

CLEAN = build_stamp.c

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
		${GLIB_CFLAGS}			\
		${GMODULE_CFLAGS} \
		${GTK_CFLAGS}

CPPFLAGS += -D_AUDACIOUS_CORE

LIBS := -L../libaudcore -laudcore \
        -L../libaudgui -laudgui \
        -L../libaudtag -laudtag \
        ${LIBS} -lm \
		${LIBINTL}						\
		${GLIB_LIBS}					\
		${GMODULE_LIBS} \
		${GTK_LIBS}
