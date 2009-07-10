include ../../buildsys.mk
include ../../extra.mk

OBJECTIVE_LIBS_NOINST = tuple_formatter_test \
	tuple_formatter_functor_test \
	tag_wma_test

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


AUDCORE_OBJS = ../libaudcore/tuple.o ../libaudcore/tuple_formatter.o \
		../libaudcore/audstrings.o ../libaudcore/tuple_compiler.o

AUDTAG_OBJS =  ../libaudtag/util.o ../libaudtag/wma/guid.o \
		../libaudtag/wma/wma.o ../libaudtag/wma/wma_fmt.o

COMMON_OBJS = test_harness.o $(AUDCORE_OBJS)

TFT_OBJS = $(COMMON_OBJS) tuple_formatter_test.o

tuple_formatter_test: $(TFT_OBJS)
	$(CC) $(LDFLAGS) $(TFT_OBJS) $(LDADD) -o $@
	@printf "%10s     %-20s\n" LINK $@
	./$@
	@printf "%10s     %-20s\n" TEST-PASS $@

TFFT_OBJS = $(COMMON_OBJS) tuple_formatter_functor_test.o
tuple_formatter_functor_test: $(TFFT_OBJS)
	$(CC) $(LDFLAGS) $(TFFT_OBJS) $(LDADD) -o $@
	@printf "%10s     %-20s\n" LINK $@
	./$@
	@printf "%10s     %-20s\n" TEST-PASS $@

TAG_WMA_OBJS = $(COMMON_OBJS) $(AUDTAG_OBJS) tag_wma_test.o
tag_wma_test: $(TAG_WMA_OBJS)
	$(CC) $(LDFLAGS) $(TAG_WMA_OBJS) $(LDADD) -o $@ -laudcore
	@printf "%10s     %-20s\n" LINK $@
	./$@
	@printf "%10s     %-20s\n" TEST-PASS $@


