LIB = ${LIB_PREFIX}audtag${LIB_SUFFIX}
LIB_MAJOR = 1
LIB_MINOR = 0

SRCS = audtag.c		\
       util.c		\
       tag_module.c	\
       wma/guid.c	\
       wma/module.c	\
       wma/wma_fmt.c	\
       wma/wma.c
INCLUDES = audtag.h 

include ../../buildsys.mk
include ../../extra.mk
includesubdir = libaudtag

CPPFLAGS += ${LIB_CPPFLAGS} ${CFLAGS} -D_AUDACIOUS_CORE -I.. -I../..
CFLAGS += ${LIB_CFLAGS} ${GLIB_CFLAGS} ${MOWGLI_CFLAGS}

LIBS += ${GLIB_LIBS} ${MOWGLI_LIBS}