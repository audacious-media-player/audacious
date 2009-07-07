LIB = ${LIB_PREFIX}audid3tag${LIB_SUFFIX}
LIB_MAJOR = 2
LIB_MINOR = 0

LIBDIR = ${plugindir}

INCLUDES = id3tag.h

SRCS = compat.c		\
       debug.c		\
       file.c		\
       frametype.c	\
       latin1.c		\
       render.c		\
       ucs4.c		\
       utf8.c		\
       version.c	\
       crc.c		\
       field.c		\
       frame.c		\
       genre.c		\
       parse.c		\
       tag.c		\
       utf16.c		\
       util.c

include ../../buildsys.mk
include ../../extra.mk

CPPFLAGS += ${LIB_CPPFLAGS} -I.. ${GLIB_CFLAGS} ${GTK_CFLAGS} \
	${MOWGLI_CFLAGS} ${LIBMCS_CFLAGS}

CFLAGS += ${LIB_CFLAGS}
LIBS += -lz ${GLIB_LIBS}
