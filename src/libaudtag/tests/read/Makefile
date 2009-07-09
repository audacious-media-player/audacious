PROG = aud-taglib-test-wma-read
SRCS = wma_read.c

include ../../../../buildsys.mk
include ../../../../extra.mk

CPPFLAGS += -I../../.. -I../../../../../ -I../../../intl \
		${GLIB_CFLAGS} \
		${GTK_CFLAGS} \
		${MOWGLI_CFLAGS}${AUDACIOUS_DEFINES} \
		${ARCH_DEFINES} \
		-D_AUDACIOUS_CORE

LIBS += -L../../../../libaudcore -laudcore -L ../../ -laudtag ${GTK_LIBS} ${MOWGLI_LIBS} ${LIBMCS_LIBS}
LDFLAGS += ${AUDLDFLAGS}