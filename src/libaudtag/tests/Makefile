SRCS =  ../audtag.c         \
		../util.c           \
        ../tag_module.c     \
        ../wma/wma.c        \
        ../wma/module.c     \
        ../wma/wma_fmt.c    \
        ../wma/guid.c
        


include ../../../buildsys.mk
include ../../../extra.mk

CPPFLAGS += -I.. -I../.. -I../intl ${GLIB_CFLAGS} ${GTK_CFLAGS} ${MOWGLI_CFLAGS}${AUDACIOUS_DEFINES} ${ARCH_DEFINES} -D_AUDACIOUS_CORE

LIBS += -L../../libaudcore -laudcore ${GTK_LIBS} ${MOWGLI_LIBS} ${LIBMCS_LIBS}
LDFLAGS += ${AUDLDFLAGS}

test-write: $(SRCS) wma_test.c
	gcc -o test_write wma_test.c $(SRCS) $(LDFLAGS) $(LIBS) $(CPPFLAGS)

test-read: $(SRCS) wmaReadTuple.c
	gcc -o test_read wmaReadTuple.c $(SRCS) $(LDFLAGS) $(LIBS) $(CPPFLAGS)
	
