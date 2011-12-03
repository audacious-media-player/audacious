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
       vfs_buffer.c \
       vfs_buffered_file.c \
       vfs_common.c \
       md5.c

INCLUDES = audio.h \
           audstrings.h \
           core.h \
           hook.h \
           index.h \
           tuple.h \
           vfs.h \
           vfs_async.h \
           vfs_buffer.h \
           vfs_buffered_file.h \
           md5.h

include ../../buildsys.mk
include ../../extra.mk

includesubdir = libaudcore

CPPFLAGS += -DHAVE_CONFIG_H ${PTHREAD_CFLAGS} ${GLIB_CFLAGS} -I.. -I../..
CFLAGS += ${LIB_CFLAGS}

LIBS += ${PTHREAD_LIBS} ${GLIB_LIBS} -lm
