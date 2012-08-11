# Short KISS instructions:
#
# - To add new languages: Add *.po target to LOCALES Make variable below
# - Update and merge .po-files: make update-po
# - Only update the template .pot-file only: make update-pot
# - Run update-potfiles.sh when any new sourcefiles with translatable strings
#   have been introduced, and then continue with 'make update-po' or
#   'make update-pot' respectively.
#
LOCALES = ast.po \
	  be.po \
	  ca.po \
	  cs.po \
	  de.po \
	  el.po \
	  es.po \
	  es_AR.po \
	  es_MX.po \
	  et.po \
	  eu.po \
	  fa_IR.po \
	  fi.po \
	  fr.po \
	  gl.po \
	  he.po \
	  hu.po \
	  id_ID.po \
	  it.po \
	  ja.po \
	  ko.po \
	  lt.po \
	  lv.po \
	  nl.po \
	  pl.po \
	  pt_BR.po \
	  pt_PT.po \
	  ro.po \
	  ru.po \
	  sk.po \
	  sr@latin.po \
	  sr.po \
	  tr.po \
	  uk.po \
	  vi.po \
	  zh_CN.po \
	  zh_TW.po

include ../extra.mk
include ../buildsys.mk

POTTEMPLATE=$(PACKAGE).pot

update-pot:
	@echo Updating $(POTTEMPLATE) ...
	xgettext --default-domain=$(PACKAGE) --language=C \
	--keyword=_ --keyword=N_ --from-code="utf-8" \
	--msgid-bugs-address="http://redmine.audacious-media-player.org/" \
	--directory=.. --files-from=POTFILES.in -o $(POTTEMPLATE)

update-po: update-pot
	@echo Updating .po -files ...
	@for lang in $(LOCALES); do echo -n "$$lang : "; msgmerge --update "$$lang" $(POTTEMPLATE); done
