include ../../mk/rules.mk
include ../../mk/init.mk

skindir = $(pkgdatadir)/Skins/Default
classic_skindir = $(pkgdatadir)/Skins/Classic

OBJECTIVE_DATA = \
	balance.png:$(skindir)		\
	cbuttons.png:$(skindir)		\
	eq_ex.png:$(skindir)		\
	eqmain.png:$(skindir)		\
	main.png:$(skindir)		\
	monoster.png:$(skindir)		\
	nums_ex.png:$(skindir)		\
	playpaus.png:$(skindir)		\
	pledit.png:$(skindir)		\
	posbar.png:$(skindir)		\
	shufrep.png:$(skindir)		\
	text.png:$(skindir)		\
	titlebar.png:$(skindir)		\
	volume.png:$(skindir)		\
	pledit.txt:$(skindir)		\
	viscolor.txt:$(skindir)		\
	skin.hints:$(skindir)		\
	balance.png:$(classic_skindir)	\
	cbuttons.png:$(classic_skindir)	\
	eq_ex.png:$(classic_skindir)	\
	eqmain.png:$(classic_skindir)	\
	main.png:$(classic_skindir)	\
	monoster.png:$(classic_skindir)	\
	nums_ex.png:$(classic_skindir)	\
	playpaus.png:$(classic_skindir)	\
	pledit.png:$(classic_skindir)	\
	posbar.png:$(classic_skindir)	\
	shufrep.png:$(classic_skindir)	\
	text.png:$(classic_skindir)	\
	titlebar.png:$(classic_skindir)	\
	volume.png:$(classic_skindir)	\
	pledit.txt:$(classic_skindir)	\
	viscolor.txt:$(classic_skindir)	\
	skin-classic.hints:$(classic_skindir)

install-posthook:
	@mv $(DESTDIR)/$(classic_skindir)/skin-classic.hints $(DESTDIR)/$(classic_skindir)/skin.hints

include ../../mk/objective.mk
