include ../buildsys.mk

pixmapsdir = $(datadir)/pixmaps

install-extra:
	y="audacious.png"; \
	for i in $$y; do \
	        ${INSTALL_STATUS}; \
		if ${MKDIR_P} ${DESTDIR}${pixmapsdir} && ${INSTALL} -m 644 $$i ${DESTDIR}${pixmapsdir}/$$i; then \
			${INSTALL_OK}; \
		else \
			${INSTALL_FAILED}; \
		fi; \
	done

uninstall-extra:
	y="audacious.png"; \
	for i in $$y; do \
		if [ -f ${DESTDIR}${pixmapsdir}/$$i ]; then \
			if rm -f ${DESTDIR}${pixmapsdir}/$$i; then \
				${DELETE_OK}; \
			else \
				${DELETE_FAILED}; \
			fi \
		fi; \
	done

