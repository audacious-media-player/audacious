SHARED_LIB = ${LIB_PREFIX}audclient${LIB_SUFFIX}
LIB_MAJOR = 2
LIB_MINOR = 0

SRCS = audctrl.c

INCLUDES = audctrl.h

include ../../buildsys.mk
include ../../extra.mk

pre-depend:
	cd ../audacious; ${MAKE} ${MFLAGS} dbus-client-bindings.h

CPPFLAGS := -I.. -I../.. \
            ${CPPFLAGS} \
            ${DBUS_CFLAGS} \
            ${GLIB_CFLAGS}

CFLAGS += ${LIB_CFLAGS}

LIBS += ${GLIB_LIBS}	\
	${DBUS_LIBS}
