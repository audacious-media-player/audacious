include ../../mk/rules.mk
include ../../mk/init.mk
include ../../mk/objective.mk

SUBDIRS = widgets glade images ui

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
	-I.. -I../..   \
	-I../intl

HEADERS = \
	input.h \
	output.h \
	playlist.h \
	playlist_container.h \
	plugin.h \
	strings.h \
	ui_fileinfopopup.h \
	ui_preferences.h \
	util.h \
	vfs.h \
	vfs_buffer.h

SOURCES = \
	build_stamp.c \
	controlsocket.c \
	dnd.c \
	dock.c \
	effect.c \
	fft.c \
	general.c \
	genevent.c \
	getopt1.c \
	getopt.c \
	glade.c \
	hints.c \
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
	playlist_manager.c \
	pluginenum.c \
	signals.c \
	strings.c \
	ui_about.c \
	ui_credits.c \
	ui_equalizer.c \
	ui_fileinfo.c \
	ui_fileinfopopup.c \
	ui_main.c \
	ui_manager.c \
	ui_playlist.c \
	ui_preferences.c \
	ui_skinselector.c \
	util.c \
	vfs_buffer.c \
	vfs_buffered_file.c \
	vfs.c \
	vfs_common.c \
	visualization.c

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

OBJECTIVE_DATA = audacious.desktop:$(datadir)/applications
