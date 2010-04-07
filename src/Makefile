SUBDIRS = libaudcore libguess libid3tag audacious libaudgui

include ../extra.mk

ifeq ($(USE_EGGSM),yes)
SUBDIRS := libeggsmclient ${SUBDIRS}
endif

ifeq ($(USE_DBUS),yes)
SUBDIRS := libaudclient audtool ${SUBDIRS}
endif

include ../buildsys.mk
