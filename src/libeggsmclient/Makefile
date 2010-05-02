ifdef PLATFORM_WIN32
platform_sources = eggsmclient-win32.c
platform_defines =
platform_libs =
else
platform_sources = eggsmclient-xsmp.c \
                   eggdesktopfile.c
platform_defines = -DEGG_SM_CLIENT_BACKEND_XSMP ${SM_CFLAGS}
platform_libs = ${SM_LIBS}
endif

STATIC_LIB_NOINST = libeggsmclient.a

SRCS = eggsmclient.c         \
       ${platform_sources}

include ../../buildsys.mk
include ../../extra.mk

CPPFLAGS += ${INCLUDES} ${GLIB_CFLAGS} ${GTK_CFLAGS} -I../.. -I. -I../audacious/platform ${platform_defines}
LIBS += ${GLIB_LIBS} ${GTK_LIBS} ${ICE_LIBS} ${platform_libs}
