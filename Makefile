.SILENT:

include mk/rules.mk
include mk/init.mk

SUBDIRS = libaudacious $(INTL_OBJECTIVE) $(SUBDIR_GUESS) audacious audtool po icons skins

include mk/objective.mk

install-posthook:
	@if test `whoami` = 'root' && test -z "$(DESTDIR)"; then \
		echo; \
		echo "WARNING:"; \
		echo "On some systems, it might be required that you run"; \
		echo "ldconfig. However, this isn't done automatically"; \
		echo "because some ldconfig versions might break the system"; \
		echo "if it's called without any parameters."; \
	fi

OBJECTIVE_DATA =							\
	audacious.pc:$(LIBDIR)/pkgconfig				\
	audacious.1:$(mandir)/man1					\
	audtool.1:$(mandir)/man1					\
	audacious/audacious.desktop:$(datadir)/applications

