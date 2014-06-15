SHARED_LIB = ${LIB_PREFIX}audcore${LIB_SUFFIX}
LIB_MAJOR = 3
LIB_MINOR = 0

SRCS = adder.cc \
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
           core.h \
           drct.h \
           equalizer.h \
           hook.h \
           i18n.h \
           index.h \
           inifile.h \
           input.h \
           interface.h \
           list.h \
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

include ../../buildsys.mk
include ../../extra.mk

includesubdir = libaudcore

LD = ${CXX}

CPPFLAGS := -I.. -I../.. \
            ${CPPFLAGS} \
            ${GLIB_CFLAGS} \
            ${GMODULE_CFLAGS} \
            ${LIBGUESS_CFLAGS} \
            ${QT_CFLAGS} \
            -DHARDCODE_BINDIR=\"${bindir}\" \
            -DHARDCODE_DATADIR=\"${datadir}/audacious\" \
            -DHARDCODE_PLUGINDIR=\"${plugindir}\" \
            -DHARDCODE_LOCALEDIR=\"${localedir}\" \
            -DHARDCODE_DESKTOPFILE=\"${datarootdir}/applications/audacious.desktop\" \
            -DHARDCODE_ICONFILE=\"${datarootdir}/pixmaps/audacious.png\"

CFLAGS += ${LIB_CFLAGS}

LIBS += -lm \
        ${GLIB_LIBS} \
        ${GMODULE_LIBS} \
        ${LIBGUESS_LIBS} \
        ${QT_LIBS}
