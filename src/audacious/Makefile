include ../../extra.mk

SUBDIRS = ${INTL_OBJECTIVE}

PROG = audacious${PROG_SUFFIX}
SRCS = af_equalizer.c						\
       auddrct.c						\
       build_stamp.c					\
       configdb.c						\
       custom_uri.c						\
       discovery.c						\
       dnd.c							\
       dock.c							\
       effect.c							\
       equalizer_flow.c						\
       eventqueue.c						\
       fft.c							\
       flow.c							\
       formatter.c						\
       general.c						\
       hints.c							\
       hook.c							\
       icons-stock.c					\
       iir.c							\
       iir_cfs.c						\
       iir_fpu.c						\
       input.c							\
       logger.c							\
       main.c							\
       mime.c							\
       output.c							\
       pixbuf_effects.c					\
       playback.c						\
       playback_evlisteners.c			\
       playlist.c						\
       playlist_container.c				\
       playlist_evlisteners.c			\
       pluginenum.c						\
       rcfile.c							\
       signals.c						\
       src_flow.c						\
       strings.c						\
       tuple.c							\
       tuple_formatter.c				\
       tuple_compiler.c					\
       skin.c							\
       ui_about.c						\
       ui_albumart.c					\
       ui_credits.c						\
       ui_equalizer.c					\
       ui_fileinfo.c					\
       ui_fileinfopopup.c				\
       ui_fileopener.c					\
       ui_jumptotrack.c					\
       ui_jumptotrack_cache.c					\
       ui_main.c						\
       ui_main_evlisteners.c			\
       ui_manager.c						\
       ui_playlist.c					\
       ui_playlist_manager.c			\
       ui_preferences.c					\
       ui_skinned_cursor.c				\
       ui_skinned_window.c				\
       ui_skinned_button.c				\
       ui_skinned_textbox.c				\
       ui_skinned_number.c				\
       ui_skinned_horizontal_slider.c	\
       ui_vis.c							\
       ui_svis.c						\
       ui_skinned_menurow.c				\
       ui_skinned_playstatus.c			\
       ui_skinned_monostereo.c			\
       ui_skinned_equalizer_slider.c	\
       ui_skinned_equalizer_graph.c		\
       ui_skinned_playlist_slider.c		\
       ui_skinned_playlist.c			\
       ui_skinselector.c				\
       ui_urlopener.c					\
       util.c							\
       vfs.c							\
       vfs_buffer.c						\
       vfs_buffered_file.c				\
       vfs_common.c						\
       visualization.c					\
       volumecontrol.c					\
       sync-menu.c

ifdef USE_DBUS
SRCS += dbus.c
endif

INCLUDES = auddrct.h		\
	   configdb.h		\
	   custom_uri.h		\
	   dbus.h		\
	   discovery.h		\
	   dbus-service.h	\
	   eventqueue.h		\
	   flow.h		\
	   formatter.h		\
	   rcfile.h		\
	   i18n.h		\
	   input.h		\
	   hook.h		\
	   main.h		\
	   mime.h		\
	   output.h		\
	   playback.h		\
	   playlist.h		\
	   playlist_container.h	\
	   plugin.h		\
	   strings.h		\
	   tuple.h		\
	   tuple_formatter.h	\
	   tuple_compiler.h	\
	   ui_fileinfopopup.h	\
	   ui_plugin_menu.h	\
	   ui_preferences.h	\
	   util.h		\
	   vfs.h		\
	   vfs_buffer.h		\
	   vfs_buffered_file.h

DATA = images/about-logo.png				\
       images/appearance.png				\
       images/audacious_eq.xpm				\
       images/audacious_player.xpm			\
       images/audacious_playlist.xpm		\
       images/audio.png						\
       images/connectivity.png				\
       images/menu_playlist.png				\
       images/menu_plugin.png				\
       images/menu_queue_toggle.png			\
       images/menu_randomize_playlist.png	\
       images/mouse.png						\
       images/playback.png					\
       images/playlist.png					\
       images/plugins.png					\
       images/replay_gain.png					\
       ui/equalizer.ui						\
       ui/mainwin.ui						\
       ui/playlist.ui						\
       ui/carbon-menubar.ui

CLEAN = build_stamp.c libaudacious.exe.a
EXT_DEPS = ../libguess/libguess.a \
           ../libSAD/libSAD.a

include ../../buildsys.mk

ifdef USE_DBUS
CPPFLAGS += -I../libaudclient
DBUS_BINDINGS = dbus-server-bindings.h dbus-client-bindings.h
LIBS += -L../libaudclient -laudclient
endif

pre-depend: ${DBUS_BINDINGS}

CPPFLAGS += -DHAVE_CONFIG_H		\
	    ${MOWGLI_CFLAGS}		\
	    ${GTK_CFLAGS}		\
	    ${BEEP_DEFINES}		\
	    ${ARCH_DEFINES}		\
	    ${DBUS_CFLAGS}		\
	    ${samplerate_CFLAGS}	\
	    ${REGEX_CFLAGS}		\
	    ${LIBMCS_CFLAGS}		\
	    ${SIMD_CFLAGS}		\
	    -D_AUDACIOUS_CORE		\
	    -I../libSAD			\
	    -I.. -I../..		\
	    -I./intl

LIBS += ${LDADD}		\
	${LIBINTL}		\
	${samplerate_LIBS}	\
	../libguess/libguess.a	\
	../libSAD/libSAD.a	\
	${GTK_LIBS}		\
	${DBUS_LIBS}		\
	${MOWGLI_LIBS}		\
	${LIBMCS_LIBS}		\
	${REGEX_LIBS}

LDFLAGS += ${PROG_IMPLIB_LDFLAGS}

build_stamp.c: 
	if [ -d ../../.hg ]; then \
		revh=`hg tip --template 'const char *svn_stamp = "#rev#:#node|short#";\n' 2>/dev/null`; \
		[ -z "$$revh" ] || echo "$$revh" > build_stamp.c; \
	fi

DBUS_BINDINGS_SOURCES = objects.xml		\
			mpris_root.xml		\
			mpris_tracklist.xml	\
			mpris_player.xml

desktop_DATA = audacious.desktop
desktopdir = ${datadir}/applicationu

dbus-server-bindings.h: ${DBUS_BINDINGS_SOURCES}
	${DBUS_BINDING_TOOL} --mode=glib-server --prefix=audacious_rc objects.xml > $@
	${DBUS_BINDING_TOOL} --mode=glib-server --prefix=mpris_root mpris_root.xml >> $@
	${DBUS_BINDING_TOOL} --mode=glib-server --prefix=mpris_tracklist mpris_tracklist.xml >> $@
	${DBUS_BINDING_TOOL} --mode=glib-server --prefix=mpris_player mpris_player.xml >> $@

dbus-client-bindings.h: ${DBUS_BINDINGS_SOURCES}
	${DBUS_BINDING_TOOL} --mode=glib-client --prefix=audacious_rc objects.xml > $@

install-extra:
	if test x"${PROG_IMPLIB_NEEDED}" = x"yes"; then \
		for i in ${PROG}; do \
			i="lib$$i.a"; \
			${INSTALL_STATUS}; \
			if ${MKDIR_P} ${DESTDIR}${libdir} && install -m 644 $$i ${DESTDIR}${libdir}/$$i; then \
				${INSTALL_OK}; \
			else \
				${INSTALL_FAILED}; \
			fi \
		done \
	fi

uninstall-extra:
	if test x"${PROG_IMPLIB_NEEDED}" = x"yes"; then \
		for i in ${PROG}; do \
			i="lib$$i.a"; \
			if test -f ${DESTDIR}${libdir}/$$i; then \
				if rm -f ${DESTDIR}${libdir}/$$i; then \
					${DELETE_OK}; \
				else \
					${DELETE_FAILED}; \
				fi \
			fi \
		done \
	fi
