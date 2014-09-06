SHARED_LIB = ${LIB_PREFIX}audqt${LIB_SUFFIX}
LIB_MAJOR = 0
LIB_MINOR = 0

SRCS = about.cc			\
       about.moc		\
       equalizer.cc		\
       equalizer.moc		\
       fileopener.cc		\
       playlist-management.cc	\
       playlist-management.moc	\
       plugin-menu.cc		\
       prefs-builder.cc		\
       prefs-plugin.cc          \
       prefs-widget.cc		\
       prefs-window.cc		\
       prefs-pluginlist-model.cc  \
       prefs-pluginlist-model.moc \
       util.cc                  \
       volumebutton.cc		\
       volumebutton.moc

INCLUDES = libaudqt.h volumebutton.h

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
