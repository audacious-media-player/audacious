include ../../extra.mk

PROG = audacious${PROG_SUFFIX}
SRCS = adder.c \
       art.c \
       chardet.c								\
       config.c \
       credits.c							\
       drct.c \
       effect.c									\
       equalizer.c								\
       equalizer_preset.c						\
       fft.c									\
       general.c \
       history.c \
       interface.c								\
       main.c									\
       mpris-signals.c \
       output.c									\
       playback.c								\
       playlist-files.c \
       playlist-new.c \
       playlist-utils.c \
       pluginenum.c								\
       plugin-registry.c \
       plugin-init.c \
       plugin-view.c \
       probe.c \
       probe-buffer.c \
       signals.c								\
       smclient.c \
       ui_plugin_menu.c \
       ui_preferences.c							\
       util.c									\
       vis_runner.c \
       visualization.c							\
       ui_albumart.c

ifeq ($(USE_DBUS),yes)
SRCS += dbus.c
endif

ifeq ($(HAVE_MSWINDOWS),yes)
SRCS += audacious.rc
endif

INCLUDES = api.h \
           api-alias-begin.h \
           api-alias-end.h \
           api-define-begin.h \
           api-define-end.h \
           dbus.h				\
           dbus-service.h		\
           debug.h \
           drct.h \
           drct-api.h \
           gtk-compat.h \
           i18n.h				\
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
       images/album.png \
       images/audacious_eq.xpm				\
       images/audacious_player.xpm			\
       images/audacious_playlist.xpm		\
       images/audio.png						\
       images/connectivity.png				\
       images/menu_playlist.png				\
       images/menu_plugin.png				\
       images/menu_queue_toggle.png			\
       images/playlist.png					\
       images/plugins.png

CLEAN = build_stamp.c dbus-client-bindings.h dbus-server-bindings.h

ifeq ($(USE_EGGSM),yes)
EXT_DEPS += ../libeggsmclient/libeggsmclient.a
EGGSM_LIBS = ../libeggsmclient/libeggsmclient.a ${SM_LIBS} ${ICE_LIBS}
EGGSM_CFLAGS =  -I../libeggsmclient ${SM_CFLAGS} ${ICE_CFLAGS}
endif

include ../../buildsys.mk

ifeq ($(USE_DBUS),yes)
DBUS_BINDINGS = dbus-server-bindings.h dbus-client-bindings.h
LIBS := -L../libaudclient -laudclient ${LIBS}
endif

pre-depend: ${DBUS_BINDINGS}

CPPFLAGS := -I.. -I../.. \
            ${CPPFLAGS} \
		${GLIB_CFLAGS}			\
		${GMODULE_LIBS} \
		${GTHREAD_CFLAGS} \
		${GTK_CFLAGS}			\
		${AUDACIOUS_DEFINES}	\
		${DBUS_CFLAGS}			\
		${REGEX_CFLAGS}			\
		${PTHREAD_CFLAGS}		\
		-D_AUDACIOUS_CORE		\
		${EGGSM_CFLAGS}			\
		${LIBGUESS_CFLAGS}

# Essential for efficient FFTs. --jlindgren
CFLAGS += -ffast-math

LIBS := -L../libaudcore -laudcore \
        -L../libaudgui -laudgui \
        -L../libaudtag -laudtag \
        ${LIBS} -lm \
		${LIBINTL}						\
		${EGGSM_LIBS}					\
		${GLIB_LIBS}					\
		${GMODULE_LIBS} \
		${GTHREAD_LIBS} \
		${GTK_LIBS}						\
		${DBUS_LIBS}					\
		${REGEX_LIBS}					\
		${PTHREAD_LIBS}					\
		${LIBGUESS_LIBS}

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
