include ../../mk/rules.mk
include ../../mk/init.mk

PICLDFLAGS = $(LIBLDFLAGS)

OBJECTIVE_LIBS = libaudacious$(SHARED_SUFFIX)
OBJECTIVE_SONAME_SUFFIX = 4

LIBADD = \
	$(GTK_LIBS) \
	$(GCONF_LIBS) \
	$(LIBMCS_LIBS)

CFLAGS += $(PICFLAGS) \
	$(GTK_CFLAGS) \
	$(GCONF_CFLAGS) \
	$(LIBMCS_CFLAGS) \
	-D_AUDACIOUS_CORE \
	-I.. -I../.. \
	-I../intl

SOURCES = \
	configdb.c \
	beepctrl.c

OBJECTS = ${SOURCES:.c=.o}

HEADERS = \
	configdb.h \
	beepctrl.h

include ../../mk/objective.mk

install-posthook:
	@mv ${DESTDIR}/${LIBDIR}/libaudacious$(SHARED_SUFFIX) ${DESTDIR}/${LIBDIR}/libaudacious$(SHARED_SUFFIX).4.0.0
	@ln -sf libaudacious$(SHARED_SUFFIX).4.0.0 \
		${DESTDIR}/${LIBDIR}/libaudacious$(SHARED_SUFFIX).4
	@ln -sf libaudacious$(SHARED_SUFFIX).4 \
		${DESTDIR}/${LIBDIR}/libaudacious$(SHARED_SUFFIX)
