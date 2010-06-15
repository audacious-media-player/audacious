include ../buildsys.mk

# These override variables in buildsys.mk and so must come after it.
PACKAGE = pixmaps
DATA = audacious.png \
       audacious.svg

install-extra:
	if test -h "${DESTDIR}${datadir}/icons/hicolor/48x48/apps/audacious.png" ; then ${RM} "${DESTDIR}${datadir}/icons/hicolor/48x48/apps/audacious.png" ; fi
	if test -h "${DESTDIR}${datadir}/icons/hicolor/scalable/apps/audacious.svg" ; then ${RM} "${DESTDIR}${datadir}/icons/hicolor/scalable/apps/audacious.svg" ; fi
	${MKDIR_P} ${DESTDIR}${datadir}/icons/hicolor/48x48/apps
	${MKDIR_P} ${DESTDIR}${datadir}/icons/hicolor/scalable/apps
	${LN_S} ../../../../pixmaps/audacious.png "${DESTDIR}${datadir}/icons/hicolor/48x48/apps/audacious.png"
	${LN_S} ../../../../pixmaps/audacious.svg "${DESTDIR}${datadir}/icons/hicolor/scalable/apps/audacious.svg"

uninstall-extra:
	if test -h "${DESTDIR}${datadir}/icons/hicolor/48x48/apps/audacious.png" ; then ${RM} "${DESTDIR}${datadir}/icons/hicolor/48x48/apps/audacious.png" ; fi
	if test -h "${DESTDIR}${datadir}/icons/hicolor/scalable/apps/audacious.svg" ; then ${RM} "${DESTDIR}${datadir}/icons/hicolor/scalable/apps/audacious.svg" ; fi
