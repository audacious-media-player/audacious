STATIC_LIB_NOINST = aud-dbus.a

SRCS = aud-dbus.c
CLEAN = aud-dbus.c aud-dbus.h

include ../../buildsys.mk
include ../../extra.mk

CPPFLAGS := -I../.. ${CPPFLAGS} ${GLIB_CFLAGS} ${GIO_CFLAGS}
LIBS := ${LIBS} ${GLIB_LIBS} ${GIO_LIBS}

pre-depend: aud-dbus.c aud-dbus.h

aud-dbus.h: aud-dbus.xml
	gdbus-codegen --interface-prefix org.atheme. --c-namespace Obj --generate-c-code aud-dbus aud-dbus.xml

aud-dbus.c: aud-dbus.h
	# nothing to do here
