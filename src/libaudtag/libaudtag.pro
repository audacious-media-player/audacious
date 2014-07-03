TEMPLATE = lib
TARGET = audtag
CONFIG += dll c++11 link_pkgconfig
PKGCONFIG += glib-2.0
VERSION = 0

DEFINES += EXPORT=''

BASE_SOURCES = audtag.cc tag_module.cc util.cc
ID3_SOURCES = id3/id3-common.cc id3/id3v1.cc id3/id3v22.cc id3/id3v24.cc
APE_SOURCES = ape/ape.cc

SOURCES = $$BASE_SOURCES $$ID3_SOURCES $$APE_SOURCES
LIBS += -L../libaudcore -laudcore

INCLUDEPATH += ..
INCLUDEPATH += ../..
