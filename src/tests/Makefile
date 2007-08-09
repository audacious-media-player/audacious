include ../../mk/rules.mk
include ../../mk/init.mk
include ../../mk/objective.mk

OBJECTIVE_LIBS_NOINST = tuple_formatter_test

LDFLAGS += $(AUDLDFLAGS)
LDADD = \
	$(DBUS_LIBS) \
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
	-I../intl -I../audacious

COMMON_OBJS = test_harness.o
TFT_OBJS = $(COMMON_OBJS) tuple_formatter_test.o ../audacious/tuple.o ../audacious/tuple_formatter.o
tuple_formatter_test: $(TFT_OBJS)
	$(CC) $(LDFLAGS) $(TFT_OBJS) $(LDADD) -o $@
	@printf "%10s     %-20s\n" LINK $@
	./$@
	@printf "%10s     %-20s\n" TEST-PASS $@
