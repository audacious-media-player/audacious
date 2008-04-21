PROG = audtool
SRCS = audtool_main.c			\
       audtool_handlers_general.c	\
       audtool_handlers_playback.c	\
       audtool_handlers_playlist.c	\
       audtool_handlers_playqueue.c	\
       audtool_handlers_vitals.c	\
       audtool_handlers_equalizer.c	\
       audtool_report.c

include ../../buildsys.mk
include ../../extra.mk

CPPFLAGS += ${CFLAGS} -I.. -I../.. -I../intl
CFLAGS += ${GTK_CFLAGS}		\
	  ${DBUS_CFLAGS}	\
	  ${AUDACIOUS_DEFINES}	\
	  ${ARCH_DEFINES}	\
	  ${MOWGLI_CFLAGS}
LIBS += ${DBUS_LIBS}			\
	-L../libaudclient -laudclient	\
	${GTK_LIBS}			\
	${MOWGLI_LIBS}
