include ../extra.mk

SUBDIRS := audacious libaudcore libaudtag

ifeq ($(USE_GTK),yes)
SUBDIRS += libaudgui
endif

ifeq ($(USE_DBUS),yes)
SUBDIRS := dbus audtool ${SUBDIRS}
endif

include ../buildsys.mk

audacious: libaudcore
libaudtag: libaudcore

ifeq ($(USE_GTK),yes)
libaudgui: libaudcore
endif

ifeq ($(USE_DBUS),yes)
audacious audtool: dbus
endif
