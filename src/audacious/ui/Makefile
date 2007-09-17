include ../../../buildsys.mk

gladexmldir = $(pkgdatadir)/ui

install-extra:
	y="*.ui"; \
	for i in $$y; do \
		${INSTALL_STATUS}; \
		if ${MKDIR_P} ${DESTDIR}${gladexmldir} && ${INSTALL} -m 644 $$i ${DESTDIR}${gladexmldir}/$$i; then \
			${INSTALL_OK}; \
		else \
			${INSTALL_FAILED}; \
		fi; \
	done

uninstall-extra:
	y="*.ui"; \
	for i in $$y; do \
		if [ -f ${DESTDIR}${gladexmldir}/$$i ]; then \
			if rm -f ${DESTDIR}${gladexmldir}/$$i; then \
				${DELETE_OK}; \
			else \
				${DELETE_FAILED}; \
			fi \
		fi; \
	done;

