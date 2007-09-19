include ../../../buildsys.mk

imagesdir = $(pkgdatadir)/images

install-extra:
	y="*.png *.xpm"; \
	for i in $$y; do \
		${INSTALL_STATUS}; \
		if ${MKDIR_P} ${DESTDIR}${imagesdir} && ${INSTALL} -m 644 $$i ${DESTDIR}${imagesdir}/$$i; then \
			${INSTALL_OK}; \
		else \
			${INSTALL_FAILED}; \
		fi; \
	done

uninstall-extra:
	y="*.png *.xpm"; \
	for i in $$y; do \
		if [ -f ${DESTDIR}${imagesdir}/$$i ]; then \
			if rm -f ${DESTDIR}${imagesdir}/$$i; then \
				${DELETE_OK}; \
			else \
				${DELETE_FAILED}; \
			fi \
		fi; \
	done;

