SHARED_LIB = ${LIB_PREFIX}audcore${LIB_SUFFIX}
LIB_MAJOR = 3
LIB_MINOR = 0

SRCS = adder.c \
       art.c \
       art-search.c \
       audio.c \
       audstrings.c \
       charset.c \
       config.c \
       drct.c \
       effect.c \
       equalizer.c \
       equalizer-preset.c \
       eventqueue.c \
       fft.c \
       history.c \
       hook.c \
       index.c \
       inifile.c \
       interface.c \
       multihash.c \
       output.c \
       playback.c \
       playlist.c \
       playlist-files.c \
       playlist-utils.c \
       plugin-init.c \
       plugin-load.c \
       plugin-registry.c \
       probe.c \
       probe-buffer.c \
       runtime.c \
       scanner.c \
       strpool.c \
       tinylock.c \
       tuple.c \
       tuple_compiler.c \
       tuple_formatter.c \
       util.c \
       vfs.c \
       vfs_async.c \
       vfs_common.c \
       vfs_local.c \
       vis-runner.c \
       visualization.c

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
           multihash.h \
           playlist.h \
           plugin.h \
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

CPPFLAGS := -I.. -I../.. \
            ${CPPFLAGS} \
            ${GLIB_CFLAGS} \
            ${GMODULE_CFLAGS} \
            ${LIBGUESS_CFLAGS} \
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
        ${LIBGUESS_LIBS}
