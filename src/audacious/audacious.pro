TEMPLATE = app
CONFIG += c++11 link_pkgconfig
PKGCONFIG += glib-2.0

SOURCES = main.cc	 \
	  signals.cc	 \
	  util.cc

DEFINES += PACKAGE=\'\"audacious\"\'
DEFINES += VERSION=\'\"3.6\"\'
DEFINES += BUILDSTAMP=\'\"qmake\"\'
DEFINES += _AUDACIOUS_CORE
DEFINES += EXPORT=''

INCLUDEPATH += ..
INCLUDEPATH += ../..

LIBS += -L../libaudcore -laudcore

