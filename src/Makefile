SUBDIRS = libeggsmclient libguess libSAD audacious libid3tag

include ../extra.mk

ifdef USE_DBUS
SUBDIRS := libaudclient audtool $(SUBDIRS)
endif

include ../buildsys.mk
