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
		pushd doc/libaudacious > /dev/null; \
			gtkdoc-scan --module=libaudacious --source-dir=../../libaudacious; \
		popd > /dev/null; \
		pushd doc/audacious > /dev/null; \
			gtkdoc-scan --module=audacious --source-dir=../../audacious --ignore-headers=intl; \
		popd > /dev/null; \
		pushd doc/libaudacious > /dev/null; \
			gtkdoc-mktmpl --module=libaudacious; \
		popd > /dev/null; \
		pushd doc/audacious > /dev/null; \
			gtkdoc-mktmpl --module=audacious; \
		popd > /dev/null; \
		pushd doc/libaudacious > /dev/null; \
			gtkdoc-mkdb --module=libaudacious --source-dir=../../libaudacious/ --output-format=xml --main-sgml-file=xml/libaudacious-main.sgml; \
		popd > /dev/null; \
		pushd doc/audacious > /dev/null; \
			gtkdoc-mkdb --module=audacious --source-dir=../../audacious/ --ignore-files=intl --output-format=xml --main-sgml-file=xml/audacious-main.sgml; \
		popd > /dev/null; \
		rm -rf doc/libaudacious/html; \
		mkdir -p doc/libaudacious/html; \
		rm -rf doc/audacious/html; \
		mkdir -p doc/audacious/html; \
		pushd doc/libaudacious/html > /dev/null; \
			gtkdoc-mkhtml libaudacious ../libaudacious-main.sgml; \
		popd > /dev/null; \
		pushd doc/audacious/html > /dev/null; \
			rm ../xml/xml; \
			ln -sf ../xml ../xml/xml; \
			gtkdoc-mkhtml audacious ../xml/audacious-main.sgml; \
		popd > /dev/null; \
		echo; \
		echo "The audacious SDK documentation was built successfully in doc/."; \
		echo; \
	);
