PROG = audtool
SRCS = main.c				\
       handlers_general.c	\
       handlers_playback.c	\
       handlers_playlist.c	\
       handlers_playqueue.c	\
       handlers_vitals.c	\
       handlers_equalizer.c	\
       report.c

include ../../buildsys.mk
include ../../extra.mk

CPPFLAGS += -I.. -I../.. \
            ${AUDACIOUS_DEFINES} \
            ${DBUS_CFLAGS} \
            ${GTK_CFLAGS}

LIBS += ${DBUS_LIBS}				\
	-L../libaudclient -laudclient	\
	${GTK_LIBS}
