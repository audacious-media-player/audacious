LIB = ${LIB_PREFIX}audcore${LIB_SUFFIX}
LIB_MAJOR = 1
LIB_MINOR = 0

SRCS = audio.c \
       audstrings.c \
       eventqueue.c \
       hook.c \
       index.c \
       strpool.c \
       tuple.c \
       tuple_compiler.c \
       tuple_formatter.c \
       vfs.c \
       vfs_async.c \
       vfs_common.c

INCLUDES = audio.h \
           audstrings.h \
           core.h \
           hook.h \
           index.h \
           tuple.h \
           vfs.h \
           vfs_async.h

include ../../buildsys.mk
include ../../extra.mk

includesubdir = libaudcore

CPPFLAGS := -I.. -I../.. \
            ${CPPFLAGS} \
            ${GLIB_CFLAGS} \
            ${PTHREAD_CFLAGS}

CFLAGS += ${LIB_CFLAGS}

LIBS += ${PTHREAD_LIBS} ${GLIB_LIBS} -lm
