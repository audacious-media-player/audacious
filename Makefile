DISTCLEAN = extra.mk

include buildsys.mk

SUBDIRS = src man po icons skins

OBJECTIVE_DATA =							\
	audacious.pc:$(LIBDIR)/pkgconfig				\
	audclient.pc:$(LIBDIR)/pkgconfig

install-extra:
	y="audacious.pc audclient.pc"; \
	for i in $$y; do \
	        ${INSTALL_STATUS}; \
		if ${MKDIR_P} ${DESTDIR}${libdir}/pkgconfig && ${INSTALL} -m 644 $$i ${DESTDIR}${libdir}/pkgconfig/$$i; then \
			${INSTALL_OK}; \
		else \
			${INSTALL_FAILED}; \
		fi; \
	done

uninstall-extra:
	y="audacious.pc audclient.pc"; \
	for i in $$y; do \
		if [ -f ${DESTDIR}${libdir}/pkgconfig/$$i ]; then \
			if rm -f ${DESTDIR}${libdir}/pkgconfig/$$i; then \
				${DELETE_OK}; \
			else \
				${DELETE_FAILED}; \
			fi \
		fi; \
	done
