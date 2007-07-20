include ../../../mk/rules.mk
include ../../../mk/init.mk

localedir = $(datadir)/locale
gettextsrcdir = $(datadir)/gettext/intl
aliaspath = $(localedir)

OBJECTIVE_LIBS_NOINST = libintl.a

DEFS += -DLOCALEDIR=\"$(localedir)\" -DLOCALE_ALIAS_PATH=\"$(aliaspath)\" \
	-DLIBDIR=\"$(libdir)\" -DIN_LIBINTL \
	-DENABLE_RELOCATABLE=1 -DIN_LIBRARY -DINSTALLDIR=\"$(libdir)\" -DINSTALLPREFIX=\"$(libdir)\" -DNO_XMALLOC \
	-Dset_relocation_prefix=libintl_set_relocation_prefix \
	-Drelocate=libintl_relocate \
	-DDEPENDS_ON_LIBICONV=1

CFLAGS += $(PICFLAGS) -I../../.. -I../.. -I. $(DEFS)

SOURCES = \
  bindtextdom.c \
  dcgettext.c \
  dgettext.c \
  gettext.c \
  finddomain.c \
  loadmsgcat.c \
  localealias.c \
  textdomain.c \
  l10nflist.c \
  explodename.c \
  dcigettext.c \
  dcngettext.c \
  dngettext.c \
  ngettext.c \
  plural.c \
  plural-exp.c \
  localcharset.c \
  relocatable.c \
  localename.c \
  log.c \
  osdep.c \
  os2compat.c \
  intl-compat.c  

OBJECTS = ${SOURCES:.c=.o}

include ../../../mk/objective.mk
