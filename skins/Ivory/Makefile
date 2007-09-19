include ../../buildsys.mk

skindir = $(pkgdatadir)/Skins/Ivory

install-extra:
	y="*.png *.txt *.hints"; \
	for i in $$y; do \
	        ${INSTALL_STATUS}; \
		if ${MKDIR_P} ${DESTDIR}${skindir} && ${INSTALL} -m 644 $$i ${DESTDIR}${skindir}/$$i; then \
			${INSTALL_OK}; \
		else \
			${INSTALL_FAILED}; \
		fi; \
	done

uninstall-extra:
	y="*.png *.txt *.hints"; \
	for i in $$y; do \
		if [ -f ${DESTDIR}${skindir}/$$i ]; then \
			if rm -f ${DESTDIR}${skindir}/$$i; then \
				${DELETE_OK}; \
			else \
				${DELETE_FAILED}; \
			fi \
		fi; \
	done
