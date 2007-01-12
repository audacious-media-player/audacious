include ../../mk/rules.mk
include ../../mk/init.mk
include ../../mk/objective.mk

beepincludedir = $(includedir)/audacious

OBJECTIVE_BINS = audtool

LDFLAGS += $(AUDLDFLAGS)
LDADD = \
	-L../libaudacious -laudacious \
	$(GTK_LIBS)      \
	$(LIBGLADE_LIBS)

CFLAGS += \
	$(GTK_CFLAGS)      \
	$(LIBGLADE_CFLAGS) \
	$(BEEP_DEFINES)    \
	$(ARCH_DEFINES)    \
	-I.. -I../..   \
	-I../intl

SOURCES = audtool.c

OBJECTS = ${SOURCES:.c=.o}

audtool: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDADD) -o $@
	@printf "%10s     %-20s\n" LINK $@
