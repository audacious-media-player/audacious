include ../../mk/rules.mk
include ../../mk/init.mk

OBJECTIVE_LIBS_NOINST = libguess.a
OBJECTIVE_SONAME_SUFFIX = 0.2.0

LDFLAGS += -Wl,-export-dynamic

CFLAGS += $(PICFLAGS)

SOURCES = \
	guess.c \
	russian_impl.c

OBJECTS = ${SOURCES:.c=.o}

include ../../mk/objective.mk
