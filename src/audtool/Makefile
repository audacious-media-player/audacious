include ${top_srcdir}/mk/rules.mk
include ${top_srcdir}/mk/init.mk
include ${top_srcdir}/mk/objective.mk

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
	-I.. -I${top_srcdir}   \
	-I../intl

SOURCES = audtool.c

OBJECTS = ${SOURCES:.c=.o}

audtool: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDADD) -o $@
	@printf "%10s     %-20s\n" LINK $@
