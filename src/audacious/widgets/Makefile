include ${top_srcdir}/mk/rules.mk
include ${top_srcdir}/mk/init.mk

OBJECTIVE_LIBS_NOINST = libwidgets.a

LDFLAGS += -Wl,-export-dynamic

CFLAGS += \
	$(GTK_CFLAGS)      \
	$(LIBGLADE_CFLAGS) \
	$(BEEP_DEFINES)    \
	$(ARCH_DEFINES)    \
	-D_AUDACIOUS_CORE  \
	-I${top_srcdir} \
	-I../..    \
	-I..	   \
	-I../../intl \
	-Wno-strict-aliasing

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

include ${top_srcdir}/mk/objective.mk

libwidgets.a: $(OBJECTS)
	$(AR) cr $@ $(OBJECTS)
