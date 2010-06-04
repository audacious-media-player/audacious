include ../../extra.mk

SUBDIRS = ${INTL_OBJECTIVE}

PROG = audacious2${PROG_SUFFIX}
SRCS = auddrct.c								\
       audconfig.c								\
       chardet.c								\
       configdb.c								\
       credits.c							\
       effect.c									\
       equalizer.c								\
       equalizer_preset.c						\
       eventqueue.c								\
       fft.c									\
       flow.c									\
       folder-add.c \
       general.c								\
       hook.c									\
       interface.c								\
       input.c									\
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
       rcfile.c									\
       signals.c								\
       ui_headless.c							\
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

INCLUDES = auddrct.h			\
           audconfig.h			\
           configdb.h			\
           dbus.h				\
           dbus-service.h		\
           equalizer_preset.h			\
           eventqueue.h			\
           flow.h				\
           rcfile.h				\
           i18n.h				\
           input.h				\
           hook.h				\
           interface.h			\
           main.h				\
           playback.h			\
           playlist_container.h	\
           plugin.h				\
           preferences.h		\
           ui_plugin_menu.h

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
       images/replay_gain.png				\
       ui/equalizer.ui						\
       ui/mainwin.ui						\
       ui/player.ui						\
       ui/playlist.ui						\
       ui/carbon-menubar.ui

CLEAN = build_stamp.c dbus-client-bindings.h dbus-server-bindings.h
EXT_DEPS = ../libguess/libguess.a

ifeq ($(USE_EGGSM),yes)
EXT_DEPS += ../libeggsmclient/libeggsmclient.a
EGGSM_LIBS = ../libeggsmclient/libeggsmclient.a ${SM_LIBS}
EGGSM_CFLAGS =  -I../libeggsmclient ${SM_CFLAGS}
endif

include ../../buildsys.mk

ifeq ($(USE_DBUS),yes)
CPPFLAGS += -I../libaudclient
DBUS_BINDINGS = dbus-server-bindings.h dbus-client-bindings.h
LIBS += -L../libaudclient -laudclient
endif

pre-depend: ${DBUS_BINDINGS}

CPPFLAGS += -DHAVE_CONFIG_H		\
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
		-I.. -I../..			\
		-I./intl			\
		-I.				\
		-I../libaudcore			\
		-I../libaudtag

LIBS += ${LDADD}						\
		-L../libaudcore -laudcore	\
		-L../libaudgui -laudgui \
		-L../libaudtag -laudtag		\
		${LIBINTL}						\
		../libguess/libguess.a			\
		${EGGSM_LIBS}					\
		${GLIB_LIBS}					\
		${GTHREAD_LIBS}					\
		${GTK_LIBS}						\
		${DBUS_LIBS}					\
		${MOWGLI_LIBS}					\
		${LIBMCS_LIBS}					\
		${REGEX_LIBS}

LDFLAGS += ${PROG_IMPLIB_LDFLAGS} ${AUDLDFLAGS}

build_stamp.c:
	echo "#include <glib.h>" > build_stamp.c ; \
	if test -d ../../.hg ; then \
		revh=`hg tip --template 'const gchar *build_stamp = "{rev}:{node|short}";\n' 2>/dev/null`; \
		[ -z "$$revh" ] || echo "$$revh" >> build_stamp.c; \
	else \
		echo "const gchar *build_stamp = \"UNSUPPORTED VERSION\";" >> build_stamp.c; \
	fi

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
