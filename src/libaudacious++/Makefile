include ../../mk/rules.mk
include ../../mk/init.mk

PICLDFLAGS = $(LIBLDFLAGS)

includedir = $(includedir)/audacious++

OBJECTIVE_LIBS = libaudacious++$(SHARED_SUFFIX)
OBJECTIVE_SONAME_SUFFIX = 1

LIBADD = \
	$(GTK_LIBS)      \
	$(GCONF_LIBS)

CXXFLAGS += $(PICFLAGS) \
	$(GTK_CFLAGS)        \
	$(GCONF_CFLAGS)      \
	-D_AUDACIOUS_CORE    \
	-I.. -I../..	      \
	-I../intl

SOURCES = configdb.cxx

OBJECTS = ${SOURCES:.cxx=.o}

HEADERS = configdb.hh

include ../../mk/objective.mk

install-posthook:
	@mv ${DESTDIR}/${LIBDIR}/libaudacious++$(SHARED_SUFFIX) ${DESTDIR}/${LIBDIR}/libaudacious++$(SHARED_SUFFIX).1.0.0
	@ln -sf ${LIBDIR}/libaudacious++$(SHARED_SUFFIX).1.0.0 \
		${DESTDIR}/${LIBDIR}/libaudacious++$(SHARED_SUFFIX).1
	@ln -sf ${LIBDIR}/libaudacious++$(SHARED_SUFFIX).1 \
		${DESTDIR}/${LIBDIR}/libaudacious++$(SHARED_SUFFIX)
