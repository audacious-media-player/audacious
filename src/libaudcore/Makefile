SHARED_LIB = ${LIB_PREFIX}audcore${LIB_SUFFIX}
LIB_MAJOR = 5
LIB_MINOR = 0

SRCS = adder.cc \
       art.cc \
       art-search.cc \
       audio.cc \
       audstrings.cc \
       charset.cc \
       config.cc \
       cue-cache.cc \
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
       logger.cc \
       mainloop.cc \
       multihash.cc \
       output.cc \
       parse.cc \
       playback.cc \
       playlist.cc \
       playlist-cache.cc \
       playlist-data.cc \
       playlist-files.cc \
       playlist-utils.cc \
       plugin-init.cc \
       plugin-load.cc \
       plugin-registry.cc \
       preferences.cc \
       probe.cc \
       probe-buffer.cc \
       ringbuf.cc \
       runtime.cc \
       scanner.cc \
       stringbuf.cc \
       strpool.cc \
       tinylock.cc \
       timer.cc \
       tuple.cc \
       tuple-compiler.cc \
       util.cc \
       vfs.cc \
       vfs_async.cc \
       vfs_local.cc \
       vis-runner.cc \
       visualization.cc

INCLUDES = audio.h \
           audstrings.h \
           drct.h \
           equalizer.h \
           export.h \
           hook.h \
           i18n.h \
           index.h \
           inifile.h \
           interface.h \
           list.h \
           mainloop.h \
           multihash.h \
           objects.h \
           playlist.h \
           plugin.h \
           plugins.h \
           preferences.h \
           probe.h \
           ringbuf.h \
           runtime.h \
           templates.h \
           tinylock.h \
           tuple.h \
           visualizer.h \
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
            ${QTCORE_CFLAGS} \
            -DHARDCODE_BINDIR=\"${bindir}\" \
            -DHARDCODE_DATADIR=\"${datadir}/audacious\" \
            -DHARDCODE_PLUGINDIR=\"${plugindir}\" \
            -DHARDCODE_LOCALEDIR=\"${localedir}\" \
            -DHARDCODE_DESKTOPFILE=\"${datarootdir}/applications/audacious.desktop\" \
            -DHARDCODE_ICONFILE=\"${datarootdir}/icons/hicolor/48x48/apps/audacious.png\" \
            -DLIBAUDCORE_BUILD

CFLAGS += ${LIB_CFLAGS}

LIBS := ../libguess/libguess.a \
        ${LIBS} -lm \
        ${GLIB_LIBS} \
        ${GMODULE_LIBS} \
        ${QTCORE_LIBS}
