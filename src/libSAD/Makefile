LIB = ${LIB_PREFIX}SAD${LIB_SUFFIX}
LIB_MAJOR = 2
LIB_MINOR = 0

SRCS = dither.c \
       dither_ops.c \
       noicegen.c

INCLUDES = libSAD.h \
           common.h \
           dither.h

include ../../buildsys.mk
include ../../extra.mk

PACKAGE = libSAD
CPPFLAGS += ${SIMD_CFLAGS} -I../..
CFLAGS += ${LIB_CFLAGS}
