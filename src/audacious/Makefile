include ../../mk/rules.mk
include ../../mk/init.mk
include ../../mk/objective.mk

SUBDIRS = widgets glade images ui

OBJECTIVE_BINS = audacious

LDFLAGS += $(AUDLDFLAGS)

LDADD = \
	-L.. $(LIBINTL) \
	-L../libaudacious -laudacious \
	$(samplerate_LIBS) \
	$(CHARDET_LIBS) \
	$(GTK_LIBS) \
	$(LIBGLADE_LIBS) \
	$(REGEX_LIBS) \
	./widgets/libwidgets.a

CFLAGS += \
	$(GTK_CFLAGS) \
	$(LIBGLADE_CFLAGS) \
	$(BEEP_DEFINES) \
	$(ARCH_DEFINES) \
	$(samplerate_CFLAGS) \
	$(REGEX_CFLAGS) \
	-D_AUDACIOUS_CORE \
	-I.. -I../.. \
	-I../intl

HEADERS = \
	formatter.h \
	rcfile.h \
	i18n.h \
	input.h \
	hook.h \
	main.h \
	output.h \
	playlist.h \
	playlist_container.h \
	plugin.h \
	strings.h \
	titlestring.h \
	ui_fileinfopopup.h \
	ui_preferences.h \
	util.h \
	vfs.h \
	vfs_buffer.h \
	xconvert.h

SOURCES = \
	build_stamp.c \
	controlsocket.c \
	dnd.c \
	dock.c \
	effect.c \
	fft.c \
	formatter.c \
	general.c \
	genevent.c \
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
	output.c \
	pixbuf_effects.c \
	playback.c \
	playlist.c \
	playlist_container.c \
	pluginenum.c \
	rcfile.c \
	signals.c \
	strings.c \
	titlestring.c \
	ui_about.c \
	ui_albumart.c \
	ui_credits.c \
	ui_equalizer.c \
	ui_fileinfo.c \
	ui_fileinfopopup.c \
	ui_fileopener.c \
	ui_jumptotrack.c \
	ui_main.c \
	ui_manager.c \
	ui_playlist.c \
	ui_playlist_manager.c \
	ui_preferences.c \
	ui_skinned_cursor.c \
	ui_skinned_window.c \
	ui_skinselector.c \
	ui_urlopener.c \
	urldecode.c \
	util.c \
	vfs.c \
	vfs_buffer.c \
	vfs_buffered_file.c \
	vfs_common.c \
	visualization.c \
	xconvert.c

OBJECTS = ${SOURCES:.c=.o}

desktop_DATA = audacious.desktop
desktopdir = $(datadir)/applications

audacious: $(OBJECTS) widgets/libwidgets.a
	$(CXX) $(LDFLAGS) $(OBJECTS) $(LDADD) -o $@ 
	@printf "%10s     %-20s\n" LINK $@

OBJECTIVE_DATA = audacious.desktop:$(datadir)/applications
