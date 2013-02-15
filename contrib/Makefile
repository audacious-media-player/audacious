include ../buildsys.mk
include ../extra.mk

ifeq ($(USE_THUNAR),yes)

install-extra:
	${MKDIR_P} ${DESTDIR}${datarootdir}/Thunar/sendto
	${INSTALL} -m 644 thunar-sendto-audacious-playlist.desktop "${DESTDIR}${datarootdir}/Thunar/sendto"

uninstall-extra:
	if test -f "${DESTDIR}${datarootdir}/Thunar/sendto/thunar-sendto-audacious-playlist.desktop" ; then ${RM} "${DESTDIR}${datarootdir}/Thunar/sendto/thunar-sendto-audacious-playlist.desktop" ; fi

endif
