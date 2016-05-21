SHARED_LIB = ${LIB_PREFIX}audtag${LIB_SUFFIX}
LIB_MAJOR = 3
LIB_MINOR = 0

SRCS = audtag.cc		\
       util.cc		\
       tag_module.cc	\
       id3/id3-common.cc \
       id3/id3v1.cc	\
       id3/id3v22.cc	\
       id3/id3v24.cc	\
       ape/ape.cc

INCLUDES = audtag.h

include ../../buildsys.mk
include ../../extra.mk

LD = ${CXX}

CPPFLAGS := -I.. -I../.. \
            ${CPPFLAGS} \
            ${GLIB_CFLAGS}

CFLAGS += ${LIB_CFLAGS}

LIBS := -L../libaudcore -laudcore \
        ${LIBS} \
        ${GLIB_LIBS}
