LIB = ${LIB_PREFIX}audclient${LIB_SUFFIX}
LIB_MAJOR = 2
LIB_MINOR = 0

SRCS = audctrl.c

INCLUDES = audctrl.h

include ../../buildsys.mk
include ../../extra.mk

pre-depend:
	cd ../audacious; ${MAKE} ${MFLAGS} dbus-client-bindings.h

CPPFLAGS = ${CFLAGS} -D_AUDACIOUS_CORE -I.. -I../..
CFLAGS += ${LIB_CFLAGS}		\
	  ${GLIB_CFLAGS}	\
	  ${DBUS_CFLAGS}
LIBS += ${GLIB_LIBS}	\
	${DBUS_LIBS}
