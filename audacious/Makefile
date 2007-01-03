include ../mk/rules.mk
include ../mk/init.mk
include ../mk/objective.mk

SUBDIRS = widgets glade images ui

beepincludedir = $(includedir)/audacious

OBJECTIVE_BINS = audacious

LDFLAGS += $(AUDLDFLAGS)

LDADD = \
	-L.. $(LTLIBINTL)     \
	-L../libaudacious -laudacious \
	$(CHARDET_LIBS) \
	$(GTK_LIBS)      \
	$(LIBGLADE_LIBS) \
	$(REGEX_LIBS)    \
	./widgets/libwidgets.a

CFLAGS += \
	$(GTK_CFLAGS)      \
	$(LIBGLADE_CFLAGS) \
	$(BEEP_DEFINES)    \
	$(ARCH_DEFINES)    \
	$(REGEX_CFLAGS)    \
	-D_AUDACIOUS_CORE \
	-I..    \
	-I../intl

HEADERS = \
	input.h \
	main.h \
	output.h \
	playlist.h \
	plugin.h \
	prefswin.h \
	util.h \
	vfs.h \
	vfs_buffer.h

SOURCES = \
	build_stamp.c \
	vfs.c \
	vfs_buffer.c \
	vfs_common.c \
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
	playlist_manager.c \
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
	ui_manager.c \
	ui_equalizer.c \
	glade.c \
	hints.c \
	about.c credits.c \
	getopt.c getopt1.c \
	iir.c \
	iir_cfs.c \
	iir_fpu.c \
	signals.c \
	strings.c \
	pixbuf_effects.c \
	memorypool.c

OBJECTS = ${SOURCES:.c=.o}

desktop_DATA = audacious.desktop
desktopdir = $(datadir)/applications

build-prehook:
	if [ -d .svn ] && [ -f build_stamp.c ]; then		\
		rm -f build_stamp.c;				\
		printf "%10s     %-20s\n" RM build_stamp.c;	\
	fi

build_stamp.c:
	if [ -d .svn ] && [ ! -f build_stamp.c ]; then		\
		( \
			echo '/* SVN repository version.'; \
			echo ' * This auto-generated file must not be included in SVN; but must'; \
			echo ' * be included in release tarballs.'; \
			echo ' */'; \
			echo '#include <glib.h>'; \
			echo -n 'const gchar *svn_stamp = "'; \
			(svnversion -n .); \
			echo '";' \
		) > build_stamp.c; \
		printf "%10s     %-20s\n" STAMP build_stamp.c;	\
	fi;

audacious: $(OBJECTS) widgets/libwidgets.a
	$(CXX) $(LDFLAGS) $(OBJECTS) $(LDADD) -o $@ 
	@printf "%10s     %-20s\n" LINK $@
