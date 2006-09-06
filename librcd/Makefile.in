include ../mk/rules.mk
include ../mk/init.mk

OBJECTIVE_LIBS_NOINST = librcd.a
OBJECTIVE_SONAME_SUFFIX = 0.1.8

LDFLAGS += -Wl,-export-dynamic

CFLAGS += $(PICFLAGS)

SOURCES = \
	librcd.c

OBJECTS = ${SOURCES:.c=.o}

include ../mk/objective.mk

libguess.a: $(OBJECTS)
	$(AR) cq $@ $(OBJECTS)
