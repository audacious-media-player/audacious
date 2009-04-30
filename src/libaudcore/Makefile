LIB = ${LIB_PREFIX}audcore${LIB_SUFFIX}
LIB_MAJOR = 1
LIB_MINOR = 0

SRCS = strings.c \
       tuple.c \
       tuple_compiler.c \
       tuple_formatter.c \
       vfs.c \
       vfs_buffer.c \
       vfs_buffered_file.c \
       vfs_common.c

INCLUDES = strings.h \
           tuple.h \
           tuple_compiler.h \
           tuple_formatter.h \
           vfs.h \
	   vfs_buffer.h \
	   vfs_buffered_file.h

include ../../buildsys.mk
include ../../extra.mk

includesubdir = libaudcore

CPPFLAGS += ${LIB_CPPFLAGS} ${CFLAGS} -D_AUDACIOUS_CORE -I.. -I../..
CFLAGS += ${LIB_CFLAGS} ${GLIB_CFLAGS} ${MOWGLI_CFLAGS}

LIBS += ${GLIB_LIBS} ${MOWGLI_LIBS}
