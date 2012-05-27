SHARED_LIB = ${LIB_PREFIX}audgui${LIB_SUFFIX}
LIB_MAJOR = 1
LIB_MINOR = 0

SRCS = about.c \
       confirm.c \
       effects-menu.c \
       equalizer.c \
       iface-menu.c \
       infopopup.c \
       infowin.c \
       init.c \
       jump-to-time.c \
       list.c \
       playlists.c \
       queue-manager.c \
       ui_fileopener.c \
       ui_jumptotrack.c \
       ui_jumptotrack_cache.c \
       icons-stock.c \
       ui_playlist_manager.c \
       urilist.c \
       url-opener.c \
       util.c

INCLUDES = libaudgui.h \
           libaudgui-gtk.h \
           list.h

include ../../buildsys.mk
include ../../extra.mk

includesubdir = libaudgui

CPPFLAGS := -I.. -I../.. \
            ${CPPFLAGS} \
            ${GLIB_CFLASG} \
            ${GTK_CFLAGS} \
            ${REGEX_CFLAGS}

CFLAGS += ${LIB_CFLAGS}

LIBS := -L../libaudcore -laudcore \
        ${LIBS} -lm \
        ${GLIB_LIBS} \
        ${GTK_LIBS} \
        ${REGEX_LIBS}
