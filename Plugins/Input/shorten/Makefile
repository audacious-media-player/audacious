include ../../../mk/rules.mk
include ../../../mk/objective.mk

OBJECTIVE_LIBS=	libshorten.so

LIBDIR= $(plugindir)/$(INPUT_PLUGIN_DIR)

SOURCES= 	avio.c aviobuf.c \
		common.c \
		cutils.c dsputil.c \
		fft.c file.c futils.c golomb.c init.c mdct.c \
		os_support.c \
		parser.c simple_idct.c shorten_decoder.c shorten_plugin.c shorten_container.c \
		utils.h utils.c

CFLAGS+=	-fPIC -DPIC -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_GNU_SOURCE -c -I../../.. -I../../../intl $(GTK_CFLAGS)

OBJECTS=${SOURCES:.c=.o}
