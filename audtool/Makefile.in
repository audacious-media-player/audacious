include ../mk/rules.mk
include ../mk/objective.mk

beepincludedir = $(includedir)/audacious

OBJECTIVE_BINS = audtool

LDFLAGS += -Wl,-export-dynamic
LDADD = \
	$(GTK_LIBS)      \
	$(LIBGLADE_LIBS) \
	-L.. $(LTLIBINTL)     \
	-L../libaudacious -laudacious

CFLAGS += \
	$(GTK_CFLAGS)      \
	$(LIBGLADE_CFLAGS) \
	$(BEEP_DEFINES)    \
	$(ARCH_DEFINES)    \
	-I..    \
	-I../intl

SOURCES = audtool.c

OBJECTS = ${SOURCES:.c=.o}

audtool: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDADD) -o $@ -Wl,-rpath,${libdir}
	@printf "%10s     %-20s\n" LINK $@
