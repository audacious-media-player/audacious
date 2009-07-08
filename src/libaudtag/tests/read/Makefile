PROG = aud-taglib-test-wma-read
SRCS =  ../../audtag.c	\
	../../util.c		\
        ../../tag_module.c	\
        ../../wma/wma.c	\
        ../../wma/module.c	\
        ../../wma/wma_fmt.c	\
        ../../wma/guid.c	\
	wma_read.c

include ../../../../buildsys.mk
include ../../../../extra.mk

CPPFLAGS += -I../../.. -I../../../../../ -I../../../intl ${GLIB_CFLAGS} ${GTK_CFLAGS} ${MOWGLI_CFLAGS}${AUDACIOUS_DEFINES} ${ARCH_DEFINES} -D_AUDACIOUS_CORE

LIBS += -L../../../../libaudcore -laudcore ${GTK_LIBS} ${MOWGLI_LIBS} ${LIBMCS_LIBS}
LDFLAGS += ${AUDLDFLAGS}
