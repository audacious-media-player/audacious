SHARED_LIB = ${LIB_PREFIX}audqt${LIB_SUFFIX}
LIB_MAJOR = 0
LIB_MINOR = 0

SRCS = about.cc			\
       about.moc		\
       equalizer.cc		\
       equalizer.moc		\
       util.cc

INCLUDES = libaudqt.h

include ../../buildsys.mk
include ../../extra.mk

includesubdir = libaudqt

LD = ${CXX}

CPPFLAGS := -I.. -I../.. \
            ${CPPFLAGS} \
            ${GLIB_CFLAGS} \
            ${QT_CFLAGS} \
            ${LIBGUESS_CFLAGS}

CFLAGS += ${LIB_CFLAGS}

LIBS := -L../libaudcore -laudcore \
        ${LIBS} -lm \
	${GLIB_LIBS} \
        ${QT_LIBS}

%.moc: %.h
	moc $< -o $@
