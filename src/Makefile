.SILENT:

include ../mk/rules.mk
include ../mk/init.mk

SUBDIRS = $(INTL_OBJECTIVE) $(SUBDIR_GUESS) $(LIBAUDCLIENT_OBJECTIVE) audacious libid3tag

include ../mk/objective.mk

depend-prehook:
	@cd audacious; $(MAKE) $@ || exit; cd ..;

