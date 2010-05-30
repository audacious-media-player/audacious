LIB = ${LIB_PREFIX}audtag${LIB_SUFFIX}
LIB_MAJOR = 1
LIB_MINOR = 0

SRCS = audtag.c		\
       util.c		\
       tag_module.c	\
       id3/id3v1.c	\
       id3/id3v24.c	\
       ape/ape.c

INCLUDES = audtag.h

include ../../buildsys.mk
include ../../extra.mk

CPPFLAGS += ${LIB_CPPFLAGS} ${GLIB_CFLAGS} ${MOWGLI_CFLAGS} -D_AUDACIOUS_CORE -I.. -I../..
CFLAGS += ${LIB_CFLAGS}
LIBS += ${GLIB_LIBS} ${MOWGLI_LIBS}
