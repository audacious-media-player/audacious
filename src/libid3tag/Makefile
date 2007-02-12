include ../../mk/rules.mk
include ../../mk/init.mk

OBJECTIVE_LIBS = libaudid3tag.so

LIBDIR = $(plugindir)

LDFLAGS += -Wl,-export-dynamic

LIBADD += $(GLIB_LIBS)

CFLAGS += $(PICFLAGS) \
	-I.. \
	$(GLIB_CFLAGS)

HEADERS = id3tag.h

SOURCES = \
	compat.c \
	debug.c \
	file.c \
	frametype.c \
	latin1.c \
	render.c \
	ucs4.c \
	utf8.c \
	version.c \
	crc.c \
	field.c \
	frame.c \
	genre.c \
	parse.c \
	tag.c \
	utf16.c \
	util.c

OBJECTS = ${SOURCES:.c=.o}

include ../../mk/objective.mk
