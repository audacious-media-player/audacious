include ../../../../mk/rules.mk
include ../../../../mk/init.mk

OBJECTIVE_LIBS_NOINST = libmp4ff.a

SOURCES = mp4ff.c mp4atom.c mp4meta.c mp4sample.c mp4util.c \
   mp4tagupdate.c drms.c

OBJECTS = ${SOURCES:.c=.o}

CFLAGS += $(PICFLAGS) -DUSE_TAGGING=1 -fsigned-char -I../../../.. -I../../../../intl

include ../../../../mk/objective.mk

libmp4ff.a: $(OBJECTS)
	$(AR) cq $@ $(OBJECTS)
