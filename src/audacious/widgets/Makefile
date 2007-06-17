include ../../../mk/rules.mk
include ../../../mk/init.mk

OBJECTIVE_LIBS_NOINST = libwidgets.a

LDFLAGS += -Wl,-export-dynamic

CFLAGS += \
	$(GTK_CFLAGS)      \
	$(DBUS_CFLAGS)     \
	$(MOWGLI_CFLAGS)   \
	$(LIBGLADE_CFLAGS) \
	$(BEEP_DEFINES)    \
	$(ARCH_DEFINES)    \
	-D_AUDACIOUS_CORE  \
	-I../../.. \
	-I../..    \
	-I..	   \
	-I../../intl

SOURCES = \
	widget.c \
	sbutton.c \
	pbutton.c \
	tbutton.c \
	textbox.c \
	hslider.c \
	menurow.c \
	monostereo.c \
	vis.c \
	svis.c \
	number.c \
	playstatus.c \
	playlist_list.c \
	playlist_slider.c \
	eq_graph.c \
	eq_slider.c \
	skin.c

OBJECTS = ${SOURCES:.c=.o}

include ../../../mk/objective.mk
