LIB = ${LIB_PREFIX}audtag${LIB_SUFFIX}
LIB_MAJOR = 1
LIB_MINOR = 0

SRCS = audtag.c		\
       util.c		\
       tag_module.c	\
       id3/id3-common.c \
       id3/id3v1.c	\
       id3/id3v22.c	\
       id3/id3v24.c	\
       ape/ape.c

INCLUDES = audtag.h

include ../../buildsys.mk
include ../../extra.mk

CPPFLAGS := -I.. -I../.. \
            ${CPPFLAGS} \
            ${GLIB_CFLAGS}

CFLAGS += ${LIB_CFLAGS}

LIBS := -L../libaudcore -laudcore \
        ${LIBS} \
        ${GLIB_LIBS}
