include ../extra.mk

SUBDIRS := audacious libaudcore libaudgui libaudtag

ifeq ($(USE_DBUS),yes)
SUBDIRS := dbus audtool ${SUBDIRS}
endif

include ../buildsys.mk

audacious libaudgui libaudtag: libaudcore

ifeq ($(USE_DBUS),yes)
audacious audtool: dbus
endif
