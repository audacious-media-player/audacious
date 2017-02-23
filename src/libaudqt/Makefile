SHARED_LIB = ${LIB_PREFIX}audqt${LIB_SUFFIX}
LIB_MAJOR = 2
LIB_MINOR = 0

SRCS = about.cc			\
       art.cc                   \
       equalizer.cc		\
       fileopener.cc		\
       infowin.cc		\
       info-widget.cc		\
       log-inspector.cc         \
       menu.cc			\
       playlist-management.cc	\
       plugin-menu.cc		\
       prefs-builder.cc		\
       prefs-plugin.cc          \
       prefs-widget.cc		\
       prefs-window.cc		\
       prefs-pluginlist-model.cc  \
       queue-manager.cc		\
       url-opener.cc		\
       util.cc			\
       volumebutton.cc

INCLUDES = export.h \
           iface.h \
           info-widget.h \
           libaudqt.h \
           menu.h

include ../../buildsys.mk
include ../../extra.mk

includesubdir = libaudqt

LD = ${CXX}

CPPFLAGS := -I.. -I../.. \
            ${CPPFLAGS} \
            ${QT_CFLAGS} \
            -DLIBAUDQT_BUILD

CFLAGS += ${LIB_CFLAGS}

LIBS := -L../libaudcore -laudcore \
        ${LIBS} -lm \
        ${QT_LIBS}

%.moc: %.h
	moc $< -o $@
