include ../mk/rules.mk
include ../mk/init.mk

PICLDFLAGS = $(LIBLDFLAGS)

beepincludedir = $(includedir)/audacious

OBJECTIVE_LIBS = libaudacious$(SHARED_SUFFIX)
OBJECTIVE_SONAME_SUFFIX = 4

LIBADD = \
	$(GTK_LIBS)      \
	$(GCONF_LIBS)

CFLAGS += $(PICFLAGS) \
	$(GTK_CFLAGS)        \
	$(GCONF_CFLAGS)      \
	-D_AUDACIOUS_CORE    \
	-I..		      \
	-I../intl

CONF_SRC = configdb_$(CONFIGDB_BACKEND).c
VFS_SRC = vfs.c vfs_common.c

SOURCES = \
	$(CONF_SRC) \
	rcfile.c \
	$(VFS_SRC) \
	beepctrl.c \
	util.c \
	formatter.c \
	titlestring.c \
	xconvert.c \
	xml_document.c

OBJECTS = ${SOURCES:.c=.o}

HEADERS = \
	vfs.h rcfile.h configdb.h \
	beepctrl.h \
	formatter.h titlestring.h xconvert.h xml_document.h

include ../mk/objective.mk

install-posthook:
	@mv ${DESTDIR}/${LIBDIR}/libaudacious$(SHARED_SUFFIX) ${DESTDIR}/${LIBDIR}/libaudacious$(SHARED_SUFFIX).4.0.0
	@ln -sf ${LIBDIR}/libaudacious$(SHARED_SUFFIX).4.0.0 \
		${DESTDIR}/${LIBDIR}/libaudacious$(SHARED_SUFFIX).4
	@ln -sf ${LIBDIR}/libaudacious$(SHARED_SUFFIX).4 \
		${DESTDIR}/${LIBDIR}/libaudacious$(SHARED_SUFFIX)
