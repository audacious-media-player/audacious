SUBDIRS = glade images

beepincludedir = $(includedir)/audacious

bin_PROGRAMS = audacious

audacious_LDFLAGS = -export-dynamic
audacious_LDADD = \
	$(GTK_LIBS)      \
	$(LIBGLADE_LIBS) \
	$(LTLIBINTL)     \
	$(top_builddir)/libaudacious/libaudacious.la \
	$(top_builddir)/libaudcore/libaudcore.la

audacious_CFLAGS = \
	$(GTK_CFLAGS)      \
	$(LIBGLADE_CFLAGS) \
	$(BEEP_DEFINES)    \
	$(ARCH_DEFINES)    \
	-I$(top_srcdir)    \
	-I$(top_srcdir)/intl

audacious_SOURCES = \
	genevent.c genevent.h \
	skin.c skin.h \
	util.c util.h \
	output.c output.h \
	fft.c fft.h \
	input.c input.h \
	effect.c effect.h \
	general.c general.h \
	visualization.c visualization.h \
	pluginenum.c pluginenum.h \
	playlist.c playlist.h \
	controlsocket.c controlsocket.h \
	dock.c dock.h \
	widget.c widget.h \
	sbutton.c sbutton.h \
	pbutton.c pbutton.h \
	tbutton.c tbutton.h \
	textbox.c textbox.h \
	menurow.c menurow.h \
	hslider.c hslider.h \
	monostereo.c monostereo.h \
	vis.c vis.h \
	svis.c svis.h \
	number.c number.h \
	playstatus.c playstatus.h \
	playlist_list.c playlist_list.h \
	playlist_slider.c playlist_slider.h \
	playlist_popup.c playlist_popup.h \
	eq_graph.c eq_graph.h \
	eq_slider.c eq_slider.h \
	main.c main.h \
	logger.c logger.h \
	mainwin.c mainwin.h \
	skinwin.c skinwin.h \
	prefswin.c prefswin.h \
	ui_playlist.c ui_playlist.h \
	equalizer.c equalizer.h \
	glade.c glade.h \
	hints.c hints.h \
	about.c credits.h \
	plugin.h \
	softvolume.c softvolume.h \
	getopt.c getopt1.c getopt.h \
	urldecode.c urldecode.h \
	dnd.h \
	debug.h \
	mkdtemp.c \
	pixmaps.h \
	iir.c iir.h \
	iir_cfs.c iir_cfs.h \
	iir_fpu.c iir_fpu.h \
	na.xpm beep_logo.xpm

beepinclude_HEADERS = plugin.h output.h input.h

desktop_DATA = audacious.desktop
desktopdir = $(datadir)/applications


BUILT_SOURCES = pixmaps.h

embedded_icons = \
	info.png playlist.png play.png


EXTRA_DIST = \
	$(embedded_icons) build.list

pixmaps.h: $(embedded_icons)
	gdk-pixbuf-csource --stream --build-list `cat build.list` > pixmaps.h
