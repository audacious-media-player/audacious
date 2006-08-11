include ../mk/rules.mk
include ../mk/objective.mk

SUBDIRS = widgets glade images

beepincludedir = $(includedir)/audacious

OBJECTIVE_BINS = audacious

LDFLAGS += -Wl,-export-dynamic
LDADD = \
	-L.. $(LTLIBINTL)     \
	-L../libaudacious -laudacious \
	../sqlite/libsqlite.a	\
	$(CHARDET_LIBS) \
	$(GTK_LIBS)      \
	$(LIBGLADE_LIBS) \
	./widgets/libwidgets.a

CFLAGS += \
	$(GTK_CFLAGS)      \
	$(LIBGLADE_CFLAGS) \
	$(BEEP_DEFINES)    \
	$(ARCH_DEFINES)    \
	-I..    \
	-I../intl

HEADERS = \
	input.h \
	output.h \
	plugin.h \
	prefswin.h

SOURCES = \
	build_stamp.c \
	genevent.c \
	util.c \
	output.c \
	fft.c \
	input.c \
	effect.c \
	general.c \
	visualization.c \
	pluginenum.c \
	playlist.c \
	playlist_container.c \
	controlsocket.c \
	dock.c \
	playback.c \
	main.c \
	logger.c \
	mainwin.c \
	skinwin.c \
	prefswin.c \
	ui_fileinfo.c \
	ui_playlist.c \
	equalizer.c \
	glade.c \
	hints.c \
	about.c credits.c \
	softvolume.c \
	getopt.c getopt1.c \
	urldecode.c \
	mkdtemp.c \
	iir.c \
	iir_cfs.c \
	iir_fpu.c

OBJECTS = ${SOURCES:.c=.o}

beepinclude_HEADERS = plugin.h output.h input.h

desktop_DATA = audacious.desktop
desktopdir = $(datadir)/applications

build_stamp.c:
	if [ -d .svn ] || [ ! -f build_stamp.c ]; then		\
		( \
			echo '/* SVN repository version.'; \
			echo ' * This auto-generated file must not be included in SVN; but must'; \
			echo ' * be included in release tarballs.'; \
			echo ' */'; \
			echo '#include <glib.h>'; \
			echo -n 'const gchar *svn_stamp = "'; \
			(cd ..; svnversion -n .); \
			echo '";' \
		) > build_stamp.c; \
	fi;

audacious: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDADD) -o $@ -Wl,-rpath,${libdir}
	@printf "%10s     %-20s\n" LINK $@
