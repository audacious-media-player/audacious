SUBDIRS = libaudcore libaudgui libaudtag audacious

include ../extra.mk

ifeq ($(USE_DBUS),yes)
SUBDIRS := dbus audtool ${SUBDIRS}
endif

include ../buildsys.mk
