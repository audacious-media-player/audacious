STATIC_LIB_NOINST = libintl.a
SRCS = bindtextdom.c	\
       dcgettext.c	\
       dgettext.c	\
       gettext.c	\
       finddomain.c	\
       loadmsgcat.c	\
       localealias.c	\
       textdomain.c	\
       l10nflist.c	\
       explodename.c	\
       dcigettext.c	\
       dcngettext.c	\
       dngettext.c	\
       ngettext.c	\
       plural.c		\
       plural-exp.c	\
       localcharset.c	\
       relocatable.c	\
       localename.c	\
       log.c		\
       osdep.c		\
       os2compat.c	\
       intl-compat.c  

include ../../../buildsys.mk
include ../../../extra.mk

localedir = ${datadir)}locale
gettextsrcdir = ${datadir}/gettext/intl
aliaspath = ${localedir}

DEFS += -DLOCALEDIR=\"${localedir}\" -DLOCALE_ALIAS_PATH=\"${aliaspath}\" \
	-DLIBDIR=\"${libdir}\" -DIN_LIBINTL \
	-DENABLE_RELOCATABLE=1 -DIN_LIBRARY -DINSTALLDIR=\"${libdir}\" -DINSTALLPREFIX=\"${libdir}\" -DNO_XMALLOC \
	-Dset_relocation_prefix=libintl_set_relocation_prefix \
	-Drelocate=libintl_relocate \
	-DDEPENDS_ON_LIBICONV=1

CPPFLAGS += -I../../.. -I../.. -I. ${DEFS}
