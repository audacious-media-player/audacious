.SILENT:

include ../mk/rules.mk
include ../mk/init.mk

SUBDIRS = $(INTL_OBJECTIVE) $(SUBDIR_GUESS)

ifdef USE_DBUS
SUBDIRS += libaudclient audtool
endif

SUBDIRS += audacious libid3tag

include ../mk/objective.mk

ifdef USE_DBUS
depend-prehook:
	@cd audacious; $(MAKE) $@ || exit; cd ..;
endif
