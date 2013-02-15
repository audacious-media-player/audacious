include ../buildsys.mk

install-extra:
	${MKDIR_P} ${DESTDIR}${datarootdir}/icons/hicolor/48x48/apps
	${MKDIR_P} ${DESTDIR}${datarootdir}/icons/hicolor/scalable/apps
	${INSTALL} -m 644 audacious.png "${DESTDIR}${datarootdir}/icons/hicolor/48x48/apps/audacious.png"
	${INSTALL} -m 644 audacious.svg "${DESTDIR}${datarootdir}/icons/hicolor/scalable/apps/audacious.svg"

uninstall-extra:
	if test -f "${DESTDIR}${datarootdir}/icons/hicolor/48x48/apps/audacious.png" ; then ${RM} "${DESTDIR}${datarootdir}/icons/hicolor/48x48/apps/audacious.png" ; fi
	if test -f "${DESTDIR}${datarootdir}/icons/hicolor/scalable/apps/audacious.svg" ; then ${RM} "${DESTDIR}${datarootdir}/icons/hicolor/scalable/apps/audacious.svg" ; fi
