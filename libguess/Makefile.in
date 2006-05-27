include ../mk/rules.mk
include ../mk/objective.mk

OBJECTIVE_LIBS = libguess.so
OBJECTIVE_SONAME_SUFFIX = 0.2.0

LDFLAGS += -Wl,-export-dynamic

CFLAGS += -fPIC -DPIC

SOURCES = \
	guess.c

OBJECTS = ${SOURCES:.c=.o}

HEADERS = \
	libguess.h

install-posthook:
	@mv ${DESTDIR}/${LIBDIR}/libguess.so ${DESTDIR}/${LIBDIR}/libguess.so.0.2.0
	@ln -sf ${LIBDIR}/libguess.so.0.2.0 \
		${DESTDIR}/${LIBDIR}/libguess.so.0
	@ln -sf ${LIBDIR}/libguess.so.0 \
		${DESTDIR}/${LIBDIR}/libguess.so
