SHARED_LIB = ${LIB_PREFIX}audgui${LIB_SUFFIX}
LIB_MAJOR = 2
LIB_MINOR = 0

SRCS = about.c \
       confirm.c \
       equalizer.c \
       infopopup.c \
       infowin.c \
       init.c \
       jump-to-time.c \
       list.c \
       menu.c \
       pixbufs.c \
       playlists.c \
       queue-manager.c \
       scaled-image.c \
       ui_fileopener.c \
       ui_jumptotrack.c \
       ui_jumptotrack_cache.c \
       ui_playlist_manager.c \
       urilist.c \
       url-opener.c \
       util.c

INCLUDES = libaudgui.h \
           libaudgui-gtk.h \
           list.h \
           menu.h

include ../../buildsys.mk
include ../../extra.mk

includesubdir = libaudgui

CPPFLAGS := -I.. -I../.. \
            ${CPPFLAGS} \
            ${GLIB_CFLASG} \
            ${GTK_CFLAGS}

CFLAGS += ${LIB_CFLAGS}

LIBS := -L../libaudcore -laudcore \
        ${LIBS} -lm \
        ${GLIB_LIBS} \
        ${GTK_LIBS}
