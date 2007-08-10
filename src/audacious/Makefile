include ../../mk/rules.mk
include ../../mk/init.mk
include ../../mk/objective.mk

SUBDIRS = $(INTL_OBJECTIVE) glade images ui

OBJECTIVE_BINS = audacious

LDFLAGS += $(AUDLDFLAGS)

LDADD = \
	$(LIBINTL) \
	$(samplerate_LIBS) \
	$(CHARDET_LIBS) \
	$(GTK_LIBS) \
	$(DBUS_LIBS) \
	$(MOWGLI_LIBS) \
	$(LIBMCS_LIBS) \
	$(LIBGLADE_LIBS) \
	$(REGEX_LIBS)

CFLAGS += \
	$(MOWGLI_CFLAGS) \
	$(GTK_CFLAGS) \
	$(LIBGLADE_CFLAGS) \
	$(BEEP_DEFINES) \
	$(ARCH_DEFINES) \
	$(DBUS_CFLAGS) \
	$(samplerate_CFLAGS) \
	$(REGEX_CFLAGS) \
	$(LIBMCS_CFLAGS) \
	-D_AUDACIOUS_CORE \
	-I.. -I../.. \
	-I./intl

HEADERS = \
	auddrct.h \
	configdb.h \
	dbus.h \
	eventqueue.h \
	formatter.h \
	rcfile.h \
	i18n.h \
	input.h \
	hook.h \
	main.h \
	mime.h \
	output.h \
	playback.h \
	playlist.h \
	playlist_container.h \
	plugin.h \
	strings.h \
	tuple.h \
	tuple_formatter.h \
	ui_fileinfopopup.h \
	ui_lastfm.h\
	ui_plugin_menu.h \
	ui_preferences.h \
	util.h \
	vfs.h \
	vfs_buffer.h \
	vfs_buffered_file.h \
	xconvert.h

SOURCES = \
	auddrct.c \
	build_stamp.c \
	configdb.c \
	discovery.c \
	dnd.c \
	dock.c \
	effect.c \
	eventqueue.c \
	fft.c \
	formatter.c \
	general.c \
	glade.c \
	hints.c \
	hook.c \
	iir.c \
	iir_cfs.c \
	iir_fpu.c \
	input.c \
	logger.c \
	main.c \
	memorypool.c \
	mime.c \
	output.c \
	pixbuf_effects.c \
	playback.c \
	playback_evlisteners.c \
	playlist.c \
	playlist_container.c \
	playlist_evlisteners.c \
	pluginenum.c \
	rcfile.c \
	signals.c \
	strings.c \
	tuple.c \
	tuple_formatter.c \
	skin.c \
	ui_about.c \
	ui_albumart.c \
	ui_credits.c \
	ui_equalizer.c \
	ui_fileinfo.c \
	ui_fileinfopopup.c \
	ui_fileopener.c \
	ui_jumptotrack.c \
	ui_lastfm.c\
	ui_main.c \
	ui_main_evlisteners.c \
	ui_manager.c \
	ui_playlist.c \
	ui_playlist_manager.c \
	ui_preferences.c \
	ui_skinned_cursor.c \
	ui_skinned_window.c \
	ui_skinned_button.c \
	ui_skinned_textbox.c \
	ui_skinned_number.c \
	ui_skinned_horizontal_slider.c \
	ui_vis.c \
	ui_svis.c \
	ui_skinned_menurow.c \
	ui_skinned_playstatus.c \
	ui_skinned_monostereo.c \
	ui_skinned_equalizer_slider.c \
	ui_skinned_equalizer_graph.c \
	ui_skinned_playlist_slider.c \
	ui_skinned_playlist.c \
	ui_skinselector.c \
	ui_urlopener.c \
	util.c \
	vfs.c \
	vfs_buffer.c \
	vfs_buffered_file.c \
	vfs_common.c \
	visualization.c \
	xconvert.c

LIBDEP = ../libguess/libguess.a

ifdef USE_DBUS
SOURCES += dbus.c
CFLAGS += -I../libaudclient
DBUS_BINDINGS = dbus-server-bindings.h dbus-client-bindings.h
OBJECTIVE_LIBS_NOINST += $(DBUS_BINDINGS)
LIBDEP += ../libaudclient/libaudclient$(SHARED_SUFFIX)
LDADD += -L../libaudclient -laudclient
endif

depend-prehook: $(DBUS_BINDINGS)

build_stamp.c: 
	if [ -d ../../.hg ]; then \
		revh=`hg tip --template 'const char *svn_stamp = "#rev#:#node|short#";\n' 2>/dev/null`; \
		[ -z "$$revh" ] || echo "$$revh" > build_stamp.c; \
		printf "%10s     %-20s\n" STAMP "build_stamp.c"; \
	fi

clean-prehook:
	if [ -d ../../.hg ]; then \
		rm -f build_stamp.c; \
	fi

DBUS_BINDINGS_SOURCES = \
	objects.xml \
	mpris_root.xml \
	mpris_tracklist.xml \
	mpris_player.xml

OBJECTS = ${SOURCES:.c=.o}

desktop_DATA = audacious.desktop
desktopdir = $(datadir)/applications

audacious: $(OBJECTS) $(LIBDEP)
	$(CXX) $(LDFLAGS) $(OBJECTS) $(LDADD) -o $@ 
	@printf "%10s     %-20s\n" LINK $@

dbus-server-bindings.h: $(DBUS_BINDINGS_SOURCES)
	$(DBUS_BINDING_TOOL) --mode=glib-server --prefix=audacious_rc objects.xml > $@
	$(DBUS_BINDING_TOOL) --mode=glib-server --prefix=mpris_root mpris_root.xml >> $@
	$(DBUS_BINDING_TOOL) --mode=glib-server --prefix=mpris_tracklist mpris_tracklist.xml >> $@
	$(DBUS_BINDING_TOOL) --mode=glib-server --prefix=mpris_player mpris_player.xml >> $@
	@printf "%10s     %-20s\n" DBUS-BIND $@

dbus-client-bindings.h: $(DBUS_BINDINGS_SOURCES)
	$(DBUS_BINDING_TOOL) --mode=glib-client --prefix=audacious_rc objects.xml > $@
	@printf "%10s     %-20s\n" DBUS-BIND $@

OBJECTIVE_DATA = audacious.desktop:$(datadir)/applications
