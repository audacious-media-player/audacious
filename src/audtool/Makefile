PROG = audtool2
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

CPPFLAGS += ${CFLAGS} -I.. -I../.. -I../intl
CFLAGS += ${GTK_CFLAGS}			\
		  ${DBUS_CFLAGS}		\
		  ${AUDACIOUS_DEFINES}	\
		  ${ARCH_DEFINES}		\
		  ${MOWGLI_CFLAGS}

LDFLAGS += ${AUDLDFLAGS}

LIBS += ${DBUS_LIBS}				\
	-L../libaudclient -laudclient	\
	${GTK_LIBS}						\
	${MOWGLI_LIBS}					\
	${LIBMCS_LIBS}

install-extra:
	if [ -h ${bindir}/audtool ] ; then rm ${bindir}/audtool ; fi
	ln -s ${bindir}/audtool2 ${bindir}/audtool

uninstall-extra:
	if [ -h ${bindir}/audtool ] ; then rm ${bindir}/audtool ; fi
