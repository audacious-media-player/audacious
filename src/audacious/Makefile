include ../../extra.mk

SUBDIRS = ${INTL_OBJECTIVE}

PROG = audacious2${PROG_SUFFIX}
SRCS = audconfig.c								\
       chardet.c								\
       configdb.c								\
       credits.c							\
       drct.c \
       effect.c									\
       equalizer.c								\
       equalizer_preset.c						\
       fft.c									\
       folder-add.c \
       general.c								\
       interface.c								\
       logger.c									\
       main.c									\
       output.c									\
       playback.c								\
       playlist_container.c						\
       playlist-new.c \
       playlist-utils.c \
       pluginenum.c								\
       plugin-registry.c \
       probe.c \
       probe-buffer.c \
       signals.c								\
       ui_plugin_menu.c \
       ui_preferences.c							\
       util.c									\
       vis_runner.c \
       visualization.c							\
       ui_albumart.c						\
       ui_misc.c

ifeq ($(USE_DBUS),yes)
SRCS += dbus.c
endif

INCLUDES = api.h \
           api-alias-begin.h \
           api-alias-end.h \
           api-define-begin.h \
           api-define-end.h \
           audconfig.h \
           configdb.h			\
           configdb-api.h \
           dbus.h				\
           dbus-service.h		\
           debug.h \
           drct.h \
           drct-api.h \
           i18n.h				\
           interface.h			\
           misc.h \
           misc-api.h \
           playlist.h \
           playlist-api.h \
           plugin.h				\
           plugins.h \
           plugins-api.h \
           preferences.h		\
           types.h

DATA = images/about-logo.png				\
       images/appearance.png				\
       images/audacious_eq.xpm				\
       images/audacious_player.xpm			\
       images/audacious_playlist.xpm		\
       images/audio.png						\
       images/blue.png						\
       images/connectivity.png				\
       images/menu_playlist.png				\
       images/menu_plugin.png				\
       images/menu_queue_toggle.png			\
       images/menu_randomize_playlist.png	\
       images/playback.png					\
       images/playlist.png					\
       images/plugins.png					\
       images/replay_gain.png

CLEAN = build_stamp.c dbus-client-bindings.h dbus-server-bindings.h

ifeq ($(USE_EGGSM),yes)
EXT_DEPS += ../libeggsmclient/libeggsmclient.a
EGGSM_LIBS = ../libeggsmclient/libeggsmclient.a ${SM_LIBS} ${ICE_LIBS}
EGGSM_CFLAGS =  -I../libeggsmclient ${SM_CFLAGS} ${ICE_CFLAGS}
endif

include ../../buildsys.mk

ifeq ($(USE_DBUS),yes)
CPPFLAGS += -I../libaudclient
DBUS_BINDINGS = dbus-server-bindings.h dbus-client-bindings.h
LIBS += -L../libaudclient -laudclient
endif

pre-depend: ${DBUS_BINDINGS}

CPPFLAGS += -std=gnu99 \
            -DHAVE_CONFIG_H \
		${MOWGLI_CFLAGS}		\
		${GLIB_CFLAGS}			\
		${GTHREAD_CFLAGS}		\
		${GTK_CFLAGS}			\
		${AUDACIOUS_DEFINES}	\
		${ARCH_DEFINES}			\
		${DBUS_CFLAGS}			\
		${REGEX_CFLAGS}			\
		${LIBMCS_CFLAGS}		\
		${SIMD_CFLAGS}			\
		-D_AUDACIOUS_CORE		\
		${EGGSM_CFLAGS}			\
		${LIBGUESS_CFLAGS}		\
		-I.. -I../..			\
		-I./intl

LIBS += ${LDADD}						\
        -lm \
		-L../libaudcore -laudcore	\
		-L../libaudgui -laudgui \
		-L../libaudtag -laudtag		\
		${LIBINTL}						\
		${EGGSM_LIBS}					\
		${GLIB_LIBS}					\
		${GTHREAD_LIBS}					\
		${GTK_LIBS}						\
		${DBUS_LIBS}					\
		${MOWGLI_LIBS}					\
		${LIBMCS_LIBS}					\
		${REGEX_LIBS}					\
		${LIBGUESS_LIBS}

LDFLAGS += ${PROG_IMPLIB_LDFLAGS} ${AUDLDFLAGS}

DBUS_BINDINGS_SOURCES = objects.xml			\
			mpris_root.xml			\
			mpris_tracklist.xml		\
			mpris_player.xml

desktop_DATA = audacious.desktop
desktopdir = ${datadir}/applications

dbus-server-bindings.h: ${DBUS_BINDINGS_SOURCES}
	${DBUS_BINDING_TOOL} --mode=glib-server --prefix=audacious_rc objects.xml > $@
	${DBUS_BINDING_TOOL} --mode=glib-server --prefix=mpris_root mpris_root.xml >> $@
	${DBUS_BINDING_TOOL} --mode=glib-server --prefix=mpris_tracklist mpris_tracklist.xml >> $@
	${DBUS_BINDING_TOOL} --mode=glib-server --prefix=mpris_player mpris_player.xml >> $@

dbus-client-bindings.h: ${DBUS_BINDINGS_SOURCES}
	${DBUS_BINDING_TOOL} --mode=glib-client --prefix=audacious_rc objects.xml > $@

install-extra:
	if test -h "${DESTDIR}${bindir}/audacious" ; then ${RM} "${DESTDIR}${bindir}/audacious" ; fi
	${LN_S} audacious2 "${DESTDIR}${bindir}/audacious"

	if test x"${PROG_IMPLIB_NEEDED}" = x"yes"; then \
		for i in ${PROG}; do \
			i="lib$$i.a"; \
			${INSTALL_STATUS}; \
			if ${MKDIR_P} "${DESTDIR}${libdir}" && ${INSTALL} -m 644 "$$i" "${DESTDIR}${libdir}/$$i"; then \
				${INSTALL_OK}; \
			else \
				${INSTALL_FAILED}; \
			fi \
		done \
	fi

uninstall-extra:
	if test -h "${DESTDIR}${bindir}/audacious" ; then ${RM} "${DESTDIR}${bindir}/audacious" ; fi

	if test x"${PROG_IMPLIB_NEEDED}" = x"yes"; then \
		for i in ${PROG}; do \
			i="lib$$i.a"; \
			if test -f "${DESTDIR}${libdir}/$$i"; then \
				if ${RM} -f "${DESTDIR}${libdir}/$$i"; then \
					${DELETE_OK}; \
				else \
					${DELETE_FAILED}; \
				fi \
			fi \
		done \
	fi
