include ../../extra.mk

PROG = audtool

SRCS =	 			\
	audtool_main.c			\
	audtool_handlers_general.c	\
	audtool_handlers_playback.c	\
	audtool_handlers_playlist.c	\
	audtool_handlers_playqueue.c	\
	audtool_handlers_vitals.c	\
	audtool_report.c

include ../../buildsys.mk

LIBS += \
	$(DBUS_LIBS) \
	-L../libaudclient -laudclient \
	$(GTK_LIBS)      \
	$(MOWGLI_LIBS)	 \
	$(LIBGLADE_LIBS)

CFLAGS += \
	$(GTK_CFLAGS)      \
	$(DBUS_CFLAGS)     \
	$(LIBGLADE_CFLAGS) \
	$(BEEP_DEFINES)    \
	$(ARCH_DEFINES)    \
	$(MOWGLI_CFLAGS)   \
	-I.. -I../..   \
	-I../intl

CPPFLAGS += $(CFLAGS)
