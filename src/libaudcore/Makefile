SHARED_LIB = ${LIB_PREFIX}audcore${LIB_SUFFIX}
LIB_MAJOR = 2
LIB_MINOR = 0

SRCS = audio.c \
       audstrings.c \
       charset.c \
       eventqueue.c \
       hook.c \
       index.c \
       inifile.c \
       multihash.c \
       strpool.c \
       tinylock.c \
       tuple.c \
       tuple_compiler.c \
       tuple_formatter.c \
       vfs.c \
       vfs_async.c \
       vfs_common.c \
       vfs_local.c

INCLUDES = audio.h \
           audstrings.h \
           core.h \
           hook.h \
           index.h \
           inifile.h \
           multihash.h \
           tinylock.h \
           tuple.h \
           vfs.h \
           vfs_async.h

include ../../buildsys.mk
include ../../extra.mk

includesubdir = libaudcore

CPPFLAGS := -I.. -I../.. \
            ${CPPFLAGS} \
            ${GLIB_CFLAGS} \
            ${LIBGUESS_CFLAGS}

CFLAGS += ${LIB_CFLAGS}

LIBS += -lm \
        ${GLIB_LIBS} \
        ${LIBGUESS_LIBS}
