MAN = audacious2.1 audtool2.1

include ../buildsys.mk

install-extra:
	${MKDIR_P} "${DESTDIR}${mandir}/${mansubdir}"
	if test -h "${DESTDIR}${mandir}/${mansubdir}/audacious.1" ; then ${RM} "${DESTDIR}${mandir}/${mansubdir}/audacious.1" ; fi
	${LN_S} audacious2.1 "${DESTDIR}${mandir}/${mansubdir}/audacious.1"
	if test -h "${DESTDIR}${mandir}/${mansubdir}/audtool.1" ; then ${RM} "${DESTDIR}${mandir}/${mansubdir}/audtool.1" ; fi
	${LN_S} audtool2.1 "${DESTDIR}${mandir}/${mansubdir}/audtool.1"

uninstall-extra:
	if test -h "${DESTDIR}${mandir}/${mansubdir}/audacious.1" ; then ${RM} "${DESTDIR}${mandir}/${mansubdir}/audacious.1" ; fi
	if test -h "${DESTDIR}${mandir}/${mansubdir}/audtool.1" ; then ${RM} "${DESTDIR}${mandir}/${mansubdir}/audtool.1" ; fi
