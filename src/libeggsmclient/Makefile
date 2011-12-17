STATIC_LIB_NOINST = libeggsmclient.a

SRCS = eggdesktopfile.c eggsmclient.c eggsmclient-xsmp.c

include ../../buildsys.mk
include ../../extra.mk

CPPFLAGS += ${GLIB_CFLAGS} ${GTK_CFLAGS} ${SM_CFLAGS} -I../.. -DEGG_SM_CLIENT_BACKEND_XSMP
LIBS += ${GLIB_LIBS} ${GTK_LIBS} ${SM_LIBS}
