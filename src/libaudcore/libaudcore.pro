# XXX: all of this kills kittens, rewrite it asap
macx {
    isEmpty(PREFIX):PREFIX = $$HOMEDIR/opt/audacious
    BINDIR = $$PREFIX/bin
    DATADIR = $$PREFIX/share
    LIBS += -liconv
}

unix {
    isEmpty(PREFIX):PREFIX = /usr/local
    BINDIR = $$PREFIX/bin
    DATADIR = $$PREFIX/share
}

TEMPLATE = lib
TARGET = audcore
CONFIG += dll c++11 link_pkgconfig
PKGCONFIG += glib-2.0 gmodule-2.0
VERSION = 0

DEFINES += EXPORT=''
DEFINES += ICONV_CONST=''
DEFINES += PACKAGE=\'\"audacious\"\'
DEFINES += VERSION=\'\"3.6\"\'
DEFINES += BUILDSTAMP=\'\"qmake\"\'
DEFINES += PLUGIN_SUFFIX=\'\"$$QMAKE_EXTENSION_SHLIB\"\'

DEFINES += HARDCODE_BINDIR=\'\"$$BINDIR\"\'
DEFINES += HARDCODE_DATADIR=\'\"$$DATADIR/audacious\"\'
DEFINES += HARDCODE_PLUGINDIR=\'\"$$PREFIX/lib/audacious\"\'
DEFINES += HARDCODE_LOCALEDIR=\'\"$$PREFIX/share/locale\"\'
DEFINES += HARDCODE_DESKTOPFILE=\'\"\"\'
DEFINES += HARDCODE_ICONFILE=\'\"$$PREFIX/pixmaps/audacious.png\"\'

#            -DHARDCODE_BINDIR=\"${bindir}\" \
#            -DHARDCODE_DATADIR=\"${datadir}/audacious\" \
#            -DHARDCODE_PLUGINDIR=\"${plugindir}\" \
#            -DHARDCODE_LOCALEDIR=\"${localedir}\" \
#            -DHARDCODE_DESKTOPFILE=\"${datarootdir}/applications/audacious.desktop\" \
#            -DHARDCODE_ICONFILE=\"${datarootdir}/pixmaps/audacious.png\"

INCLUDEPATH += ..
INCLUDEPATH += ../..

SOURCES = adder.cc \
       art.cc \
       art-search.cc \
       audio.cc \
       audstrings.cc \
       charset.cc \
       config.cc \
       drct.cc \
       effect.cc \
       equalizer.cc \
       equalizer-preset.cc \
       eventqueue.cc \
       fft.cc \
       history.cc \
       hook.cc \
       index.cc \
       inifile.cc \
       interface.cc \
       list.cc \
       mainloop.cc \
       multihash.cc \
       output.cc \
       playback.cc \
       playlist.cc \
       playlist-files.cc \
       playlist-utils.cc \
       plugin-init.cc \
       plugin-load.cc \
       plugin-registry.cc \
       probe.cc \
       probe-buffer.cc \
       runtime.cc \
       scanner.cc \
       stringbuf.cc \
       strpool.cc \
       tinylock.cc \
       tuple.cc \
       tuple-compiler.cc \
       util.cc \
       vfs.cc \
       vfs_async.cc \
       vfs_common.cc \
       vfs_local.cc \
       vis-runner.cc \
       visualization.cc

INCLUDES = audio.h \
           audstrings.h \
           drct.h \
           equalizer.h \
           hook.h \
           i18n.h \
           index.h \
           inifile.h \
           input.h \
           interface.h \
           list.h \
           mainloop.h \
           multihash.h \
           objects.h \
           playlist.h \
           plugin.h \
           plugin-declare.h \
           plugins.h \
           preferences.h \
           probe.h \
           runtime.h \
           tinylock.h \
           tuple.h \
           vfs.h \
           vfs_async.h
