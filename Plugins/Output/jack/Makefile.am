
libjackout_la_SOURCES = jack.c configure.c bio2jack.h bio2jack.c xconvert.h


lib_LTLIBRARIES = libjackout.la


libdir = $(plugindir)/$(OUTPUT_PLUGIN_DIR)

AM_CFLAGS = @GTK_CFLAGS@ @JACK_CFLAGS@ -Wall @SAMPLERATE_CFLAGS@
LIBS = @GTK_LIBS@ @JACK_LIBS@ @SAMPLERATE_LIBS@ -lpthread
libjackout_la_LDFLAGS = -module -avoid-version
