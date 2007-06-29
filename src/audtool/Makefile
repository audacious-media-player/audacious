include ../../mk/rules.mk
include ../../mk/init.mk
include ../../mk/objective.mk

OBJECTIVE_BINS = audtool

LDFLAGS += $(AUDLDFLAGS)
LDADD = \
	$(DBUS_LIBS) \
	-L../libaudclient -laudclient \
	$(GTK_LIBS)      \
	$(MOWGLI_LIBS)	 \
	$(LIBGLADE_LIBS)

CFLAGS += \
	$(GTK_CFLAGS)      \
	$(DBUS_CFLAGS)     \
	$(LIBGLADE_CFLAGS) \
	$(BEEP_DEFINES)    \
	$(ARCH_DEFINES)    \
	$(MOWGLI_CFLAGS)   \
	-I.. -I../..   \
	-I../intl

SOURCES =	 			\
	audtool_main.c			\
	audtool_handlers_general.c	\
	audtool_handlers_vitals.c

OBJECTS = ${SOURCES:.c=.o}

audtool: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDADD) -o $@
	@printf "%10s     %-20s\n" LINK $@
