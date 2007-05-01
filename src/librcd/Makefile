include ../../mk/rules.mk
include ../../mk/init.mk

OBJECTIVE_LIBS_NOINST = librcd.a

LDFLAGS += -Wl,-export-dynamic

CFLAGS += $(PICFLAGS)

SOURCES = \
	librcd.c

OBJECTS = ${SOURCES:.c=.o}

include ../../mk/objective.mk
