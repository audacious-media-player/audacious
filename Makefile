SUBDIRS = src man pixmaps po

DISTCLEAN = audacious.pc buildsys.mk config.h config.log config.status extra.mk

DATA = AUTHORS COPYING

include buildsys.mk

install-extra:
	for i in audacious.pc; do \
	        ${INSTALL_STATUS}; \
		if ${MKDIR_P} ${DESTDIR}${pkgconfigdir} && ${INSTALL} -m 644 $$i ${DESTDIR}${pkgconfigdir}/$$i; then \
			${INSTALL_OK}; \
		else \
			${INSTALL_FAILED}; \
		fi; \
	done
	for i in audacious.desktop; do \
		${INSTALL_STATUS}; \
		if ${MKDIR_P} ${DESTDIR}${datarootdir}/applications && ${INSTALL} -m 644 $$i ${DESTDIR}${datarootdir}/applications/$$i; then \
			${INSTALL_OK}; \
		else \
			${INSTALL_FAILED}; \
		fi \
	done
	if [ -f ${DESTDIR}${datadir}/audacious/Skins/Default/balance.png ]; then \
		rm -f ${DESTDIR}${datadir}/audacious/Skins/Default/balance.png; \
	fi

uninstall-extra:
	for i in audacious.pc; do \
		if [ -f ${DESTDIR}${pkgconfigdir}/$$i ]; then \
			if rm -f ${DESTDIR}${pkgconfigdir}/$$i; then \
				${DELETE_OK}; \
			else \
				${DELETE_FAILED}; \
			fi \
		fi; \
	done
	for i in audacious.desktop; do \
		if test -f ${DESTDIR}${datarootdir}/applications/$$i; then \
			if rm -f ${DESTDIR}${datarootdir}/applications/$$i; then \
				${DELETE_OK}; \
			else \
				${DELETE_FAILED}; \
			fi \
		fi \
	done
