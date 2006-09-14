.SILENT:

include mk/rules.mk
include mk/init.mk

SUBDIRS = Plugins libaudacious intl $(SUBDIR_GUESS) sqlite audacious audtool po icons skin

include mk/objective.mk

install-posthook:
	@if test `whoami` = 'root' && test -z "$(DESTDIR)"; then \
		echo "[running ldconfig to update system library cache]"; \
		/sbin/ldconfig; \
		echo "[system library cache updated]"; \
	fi

OBJECTIVE_DATA =							\
	audacious.pc:$(LIBDIR)/pkgconfig				\
	audacious.1:$(mandir)/man1					\
	audacious/audacious.desktop:$(datadir)/applications

