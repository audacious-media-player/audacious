LIB = ${LIB_PREFIX}audclient${LIB_SUFFIX}
LIB_MAJOR = 1
LIB_MINOR = 0

SRCS = audctrl.c

INCLUDES = audctrl.h

include ../../buildsys.mk
include ../../extra.mk

pre-depend:
	cd ../audacious; ${MAKE} ${MFLAGS} dbus-client-bindings.h

CPPFLAGS = ${LIB_CPPFLAGS} ${CFLAGS} -D_AUDACIOUS_CORE -I.. -I../.. -I../intl
CFLAGS += ${LIB_CFLAGS}		\
	  ${MOWGLI_CFLAGS}	\
	  ${GTK_CFLAGS}		\
	  ${GCONF_CFLAGS}	\
	  ${DBUS_CFLAGS}	\
	  ${LIBMCS_CFLAGS}
LIBS += ${GLIB_LIBS}	\
	${DBUS_LIBS}	\
	${MOWGLI_LIBS}	\
	${GCONF_LIBS}	\
	${LIBMCS_LIBS}
