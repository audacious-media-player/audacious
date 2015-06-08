STATIC_LIB_NOINST = libguess.a

SRCS = dfa.c \
       guess.c \
       guess_impl.c

include ../../buildsys.mk
include ../../extra.mk

CPPFLAGS := -I../.. ${CPPFLAGS} -DLIBGUESS_CORE

CFLAGS += ${LIB_CFLAGS} -Wno-unused-variable
