LIB = ${LIB_PREFIX}audtag${LIB_SUFFIX}
LIB_MAJOR = 1
LIB_MINOR = 0

SRCS = audtag.c		\
       util.c		\
       tag_module.c	\
       wma/guid.c	\
       wma/wma.c	\
       id3/id3.c	\
       ape/ape.c	\
       aac/aac.c
INCLUDES = audtag.h util.h

include ../../buildsys.mk
include ../../extra.mk
includesubdir = libaudtag

CPPFLAGS += ${LIB_CPPFLAGS} ${GLIB_CFLAGS} ${MOWGLI_CFLAGS} -D_AUDACIOUS_CORE -I.. -I../..
CFLAGS += ${LIB_CFLAGS}
LIBS += ${GLIB_LIBS} ${MOWGLI_LIBS}
