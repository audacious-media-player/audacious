include ../../buildsys.mk
include ../../extra.mk

OBJECTIVE_LIBS_NOINST = tuple_formatter_test \
	tuple_formatter_functor_test \
	wma_test

LDFLAGS += $(AUDLDFLAGS)
LDADD = \
	$(DBUS_LIBS) \
	$(GTK_LIBS)      \
	$(MOWGLI_LIBS)

CFLAGS += \
	$(GTK_CFLAGS)      \
	$(DBUS_CFLAGS)     \
	$(AUDACIOUS_DEFINES)    \
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

TFFT_OBJS = $(COMMON_OBJS) tuple_formatter_functor_test.o ../audacious/tuple.o ../audacious/tuple_formatter.o
tuple_formatter_functor_test: $(TFFT_OBJS)
	$(CC) $(LDFLAGS) $(TFFT_OBJS) $(LDADD) -o $@
	@printf "%10s     %-20s\n" LINK $@
	./$@
	@printf "%10s     %-20s\n" TEST-PASS $@

WMA_OBJS = $(COMMON_OBJS) wma_test.o ../libaudtag/wma.o 
wma_test: $(WMA_OBJS)
	$(CC) $(LDFLAGS) $(WMA_OBJS) $(LDADD) -o $@
	@printf "%10s     %-20s\n" LINK $@
	./$@
	@printf "%10s     %-20s\n" TEST-PASS $@


