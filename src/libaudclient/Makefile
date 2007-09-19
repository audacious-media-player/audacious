include ../../extra.mk

LIB = ${LIB_PREFIX}audclient${LIB_SUFFIX}
LIB_MAJOR = 1
LIB_MINOR = 0

SRCS = audctrl.c

INCLUDES = audctrl.h

include ../../buildsys.mk

depend-hook:
	cd ../audacious; make dbus-client-bindings.h

LIBS += \
	$(GLIB_LIBS) \
	$(DBUS_LIBS) \
	$(MOWGLI_LIBS) \
	$(GCONF_LIBS) \
	$(LIBMCS_LIBS)

CFLAGS += $(LIB_CFLAGS) \
	$(MOWGLI_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GCONF_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(LIBMCS_CFLAGS) \
	-D_AUDACIOUS_CORE \
	-I.. -I../.. \
	-I../intl

CPPFLAGS = ${CFLAGS}
