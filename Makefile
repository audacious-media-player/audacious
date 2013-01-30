SUBDIRS = src man pixmaps po

DISTCLEAN = audacious.pc audclient.pc buildsys.mk config.h config.log config.status extra.mk

DATA = AUTHORS COPYING

include buildsys.mk

install-extra:
	for i in audacious.pc audclient.pc; do \
	        ${INSTALL_STATUS}; \
		if ${MKDIR_P} ${DESTDIR}${pkgconfigdir} && ${INSTALL} -m 644 $$i ${DESTDIR}${pkgconfigdir}/$$i; then \
			${INSTALL_OK}; \
		else \
			${INSTALL_FAILED}; \
		fi; \
	done
	for i in audacious.desktop; do \
		${INSTALL_STATUS}; \
		if ${MKDIR_P} ${DESTDIR}${datadir}/applications && ${INSTALL} -m 644 $$i ${DESTDIR}${datadir}/applications/$$i; then \
			${INSTALL_OK}; \
		else \
			${INSTALL_FAILED}; \
		fi \
	done
	if [ -f ${DESTDIR}${datadir}/audacious/Skins/Default/balance.png ]; then \
		rm -f ${DESTDIR}${datadir}/audacious/Skins/Default/balance.png; \
	fi

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
	for i in audacious.desktop; do \
		if test -f ${DESTDIR}${datadir}/applications/$$i; then \
			if rm -f ${DESTDIR}${datadir}/applications/$$i; then \
				${DELETE_OK}; \
			else \
				${DELETE_FAILED}; \
			fi \
		fi \
	done
