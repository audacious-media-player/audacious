SUBDIRS = libguess audacious libid3tag libSAD

include ../extra.mk

ifdef USE_DBUS
SUBDIRS := libaudclient audtool $(SUBDIRS)
endif

include ../buildsys.mk
