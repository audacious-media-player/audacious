include ../../mk/rules.mk
include ../../mk/init.mk

OBJECTIVE_LIBS_NOINST = libnewui.a

LDFLAGS += -Wl,-export-dynamic

CFLAGS += \
	$(GTK_CFLAGS)      \
	$(LIBGLADE_CFLAGS) \
	$(BEEP_DEFINES)    \
	$(ARCH_DEFINES)    \
	-D_AUDACIOUS_CORE  \
	-I../..    \
	-I..	   \
	-I../../intl \
	-Wno-strict-aliasing

SOURCES = \
	newui_window.c

OBJECTS = ${SOURCES:.c=.o}

include ../../mk/objective.mk

libnewui.a: $(OBJECTS)
	$(AR) cq $@ $(OBJECTS)
