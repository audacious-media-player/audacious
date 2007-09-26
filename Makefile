SUBDIRS = src man pixmaps po
DATA = Skins/Classic/balance.png	\
       Skins/Classic/cbuttons.png	\
       Skins/Classic/eq_ex.png		\
       Skins/Classic/eqmain.png		\
       Skins/Classic/main.png		\
       Skins/Classic/monoster.png	\
       Skins/Classic/nums_ex.png	\
       Skins/Classic/playpaus.png	\
       Skins/Classic/pledit.png		\
       Skins/Classic/pledit.txt		\
       Skins/Classic/posbar.png		\
       Skins/Classic/shufrep.png	\
       Skins/Classic/skin-classic.hints	\
       Skins/Classic/skin.hints		\
       Skins/Classic/text.png		\
       Skins/Classic/titlebar.png	\
       Skins/Classic/viscolor.txt	\
       Skins/Classic/volume.png		\
       Skins/Default/balance.png	\
       Skins/Default/cbuttons.png	\
       Skins/Default/eq_ex.png		\
       Skins/Default/eqmain.png		\
       Skins/Default/main.png		\
       Skins/Default/monoster.png	\
       Skins/Default/nums_ex.png	\
       Skins/Default/playpaus.png	\
       Skins/Default/pledit.png		\
       Skins/Default/pledit.txt		\
       Skins/Default/posbar.png		\
       Skins/Default/shufrep.png	\
       Skins/Default/skin-classic.hints	\
       Skins/Default/skin.hints		\
       Skins/Default/text.png		\
       Skins/Default/titlebar.png	\
       Skins/Default/viscolor.txt	\
       Skins/Default/volume.png		\
       Skins/Ivory/balance.png		\
       Skins/Ivory/cbuttons.png		\
       Skins/Ivory/eq_ex.png		\
       Skins/Ivory/eqmain.png		\
       Skins/Ivory/main.png		\
       Skins/Ivory/monoster.png		\
       Skins/Ivory/nums_ex.png		\
       Skins/Ivory/playpaus.png		\
       Skins/Ivory/pledit.png		\
       Skins/Ivory/pledit.txt		\
       Skins/Ivory/posbar.png		\
       Skins/Ivory/shufrep.png		\
       Skins/Ivory/skin.hints		\
       Skins/Ivory/text.png		\
       Skins/Ivory/titlebar.png		\
       Skins/Ivory/viscolor.txt		\
       Skins/Ivory/volume.png		\
       Skins/Osmosis/balance.png	\
       Skins/Osmosis/cbuttons.png	\
       Skins/Osmosis/eq_ex.png		\
       Skins/Osmosis/eqmain.png		\
       Skins/Osmosis/main.png		\
       Skins/Osmosis/monoster.png	\
       Skins/Osmosis/nums_ex.png	\
       Skins/Osmosis/playpaus.png	\
       Skins/Osmosis/pledit.png		\
       Skins/Osmosis/pledit.txt		\
       Skins/Osmosis/posbar.png		\
       Skins/Osmosis/shufrep.png	\
       Skins/Osmosis/skin.hints		\
       Skins/Osmosis/text.png		\
       Skins/Osmosis/titlebar.png	\
       Skins/Osmosis/viscolor.txt	\
       Skins/Osmosis/volume.png		\
       Skins/TinyPlayer/balance.png	\
       Skins/TinyPlayer/cbuttons.png	\
       Skins/TinyPlayer/eq_ex.png	\
       Skins/TinyPlayer/eqmain.png	\
       Skins/TinyPlayer/main.png	\
       Skins/TinyPlayer/monoster.png	\
       Skins/TinyPlayer/nums_ex.png	\
       Skins/TinyPlayer/playpaus.png	\
       Skins/TinyPlayer/pledit.png	\
       Skins/TinyPlayer/pledit.txt	\
       Skins/TinyPlayer/posbar.png	\
       Skins/TinyPlayer/shufrep.png	\
       Skins/TinyPlayer/skin.hints	\
       Skins/TinyPlayer/text.png	\
       Skins/TinyPlayer/titlebar.png	\
       Skins/TinyPlayer/viscolor.txt	\
       Skins/TinyPlayer/volume.png	\
       Skins/Refugee/cbuttons.png	\
       Skins/Refugee/eq_ex.png		\
       Skins/Refugee/eqmain.png		\
       Skins/Refugee/main.png		\
       Skins/Refugee/monoster.png	\
       Skins/Refugee/nums_ex.png	\
       Skins/Refugee/playpaus.png	\
       Skins/Refugee/pledit.png		\
       Skins/Refugee/pledit.txt		\
       Skins/Refugee/posbar.png		\
       Skins/Refugee/shufrep.png	\
       Skins/Refugee/skin.hints		\
       Skins/Refugee/text.png		\
       Skins/Refugee/titlebar.png	\
       Skins/Refugee/viscolor.txt	\
       Skins/Refugee/volume.png		\
       applications/audacious.desktop

DISTCLEAN = extra.mk

include buildsys.mk

install-extra:
	for i in audacious.pc audclient.pc; do \
	        ${INSTALL_STATUS}; \
		if ${MKDIR_P} ${DESTDIR}${libdir}/pkgconfig && ${INSTALL} -m 644 $$i ${DESTDIR}${libdir}/pkgconfig/$$i; then \
			${INSTALL_OK}; \
		else \
			${INSTALL_FAILED}; \
		fi; \
	done

uninstall-extra:
	for i in audacious.pc audclient.pc; do \
		if [ -f ${DESTDIR}${libdir}/pkgconfig/$$i ]; then \
			if rm -f ${DESTDIR}${libdir}/pkgconfig/$$i; then \
				${DELETE_OK}; \
			else \
				${DELETE_FAILED}; \
			fi \
		fi; \
	done
