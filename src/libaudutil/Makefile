LIB = ${LIB_PREFIX}audutil${LIB_SUFFIX}
LIB_MAJOR = 1
LIB_MINOR = 0

SRCS = md5.c

INCLUDES = audutil.h

include ../../buildsys.mk
include ../../extra.mk

CPPFLAGS = ${LIB_CPPFLAGS} ${CFLAGS} -D_AUDACIOUS_CORE -I.. -I../..
CFLAGS += ${LIB_CFLAGS} ${GLIB_CFLAGS} ${MOWGLI_CFLAGS}

LIBS += ${GLIB_LIBS} ${MOWGLI_LIBS}
