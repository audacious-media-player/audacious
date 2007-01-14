.SILENT:

include mk/rules.mk
include mk/init.mk

SUBDIRS = src man po icons skins

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
	src/audacious/audacious.desktop:$(datadir)/applications

build-posthook:
	@( \
		echo; \
		echo "Now type '$(MAKE) documentation-build' to build the audacious SDK docs."; \
		echo "This will require GTK-DOC to be installed."; \
		echo; \
	);

documentation-build:
	( \
		if [ ! -d doc ]; then \
			mkdir -p doc/libaudacious/xml; \
			mkdir -p doc/audacious/xml; \
		fi; \
		cd doc/libaudacious; \
			gtkdoc-scan --module=libaudacious --source-dir=../../src/libaudacious; \
		cd ../..; \
		cd doc/audacious; \
			gtkdoc-scan --module=audacious --source-dir=../../src/audacious --ignore-headers=intl; \
		cd ../..; \
		cd doc/libaudacious; \
			gtkdoc-mktmpl --module=libaudacious; \
		cd ../..; \
		cd doc/audacious; \
			gtkdoc-mktmpl --module=audacious; \
		cd ../..; \
		cd doc/libaudacious; \
			gtkdoc-mkdb --module=libaudacious --source-dir=../../src/libaudacious/ --output-format=xml --main-sgml-file=xml/libaudacious-main.sgml; \
		cd ../..; \
		cd doc/audacious; \
			gtkdoc-mkdb --module=audacious --source-dir=../../src/audacious/ --ignore-files=intl --output-format=xml --main-sgml-file=xml/audacious-main.sgml; \
		cd ../..; \
		rm -rf doc/libaudacious/html; \
		mkdir -p doc/libaudacious/html; \
		rm -rf doc/audacious/html; \
		mkdir -p doc/audacious/html; \
		cd doc/libaudacious/html; \
			gtkdoc-mkhtml libaudacious ../libaudacious-main.sgml; \
		cd ../../..; \
		cd doc/audacious/html; \
			rm ../xml/xml; \
			ln -sf ../xml ../xml/xml; \
			gtkdoc-mkhtml audacious ../xml/audacious-main.sgml; \
		cd ../../..; \
		echo; \
		echo "The audacious SDK documentation was built successfully in doc/."; \
		echo; \
	);
