STATIC_LIB_NOINST = libSAD.a
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
