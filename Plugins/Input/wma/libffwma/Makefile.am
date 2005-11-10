noinst_LIBRARIES=	libffwma.a

libffwma_a_SOURCES=	allcodecs.c allformats.c \
			asf.c avcodec.h avformat.h \
			avi.h avio.c avio.h aviobuf.c \
			bswap.h common.c common.h \
			cutils.c dsputil.c dsputil.h \
			fft.c file.c futils.c mdct.c \
			os_support.c os_support.h \
			parser.c simple_idct.c \
			simple_idct.h utils.c wmadata.h \
			wmadec.c
libffwma_a_CFLAGS=	 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_GNU_SOURCE -c
