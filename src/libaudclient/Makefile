include ../../mk/rules.mk
include ../../mk/init.mk

PICLDFLAGS = $(LIBLDFLAGS)

OBJECTIVE_LIBS = libaudclient$(SHARED_SUFFIX)
OBJECTIVE_SONAME_SUFFIX = 1
LIBAUDACIOUS_SUFFIX = 1.0.0

LIBADD = \
	$(GTK_LIBS) \
	$(DBUS_LIBS) \
	$(MOWGLI_LIBS) \
	$(GCONF_LIBS) \
	$(LIBMCS_LIBS)

CFLAGS += $(PICFLAGS) \
	$(MOWGLI_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GCONF_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(LIBMCS_CFLAGS) \
	-D_AUDACIOUS_CORE \
	-I.. -I../.. \
	-I../intl

SOURCES = \
	audctrl.c

OBJECTS = ${SOURCES:.c=.o}

HEADERS = \
	audctrl.h

include ../../mk/objective.mk

install-posthook:
	@mv ${DESTDIR}/${LIBDIR}/libaudclient$(SHARED_SUFFIX) ${DESTDIR}/${LIBDIR}/libaudclient$(SHARED_SUFFIX).$(LIBAUDACIOUS_SUFFIX)
	@ln -sf libaudclient$(SHARED_SUFFIX).$(LIBAUDACIOUS_SUFFIX) \
		${DESTDIR}/${LIBDIR}/libaudclient$(SHARED_SUFFIX).1
	@ln -sf libaudclient$(SHARED_SUFFIX).1 \
		${DESTDIR}/${LIBDIR}/libaudclient$(SHARED_SUFFIX)
