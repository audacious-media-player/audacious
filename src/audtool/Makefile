PROG = audtool
SRCS = main.c				\
       handlers_general.c	\
       handlers_playback.c	\
       handlers_playlist.c	\
       handlers_playqueue.c	\
       handlers_vitals.c	\
       handlers_equalizer.c	\
       report.c \
       wrappers.c

EXT_DEPS += ../dbus/aud-dbus.a

include ../../buildsys.mk
include ../../extra.mk

CPPFLAGS := -I../.. -I../dbus ${CPPFLAGS} ${GLIB_CFLAGS} ${GIO_CFLAGS}
LIBS := ../dbus/aud-dbus.a ${LIBS} ${GLIB_LIBS} ${GIO_LIBS}
