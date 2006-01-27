if ENABLE_AMIDIPLUG

lib_LTLIBRARIES = libamidi-plug.la

endif

noinst_HEADERS = amidi-plug.h i_seq.h i_midi.h i_configure.h \
                 i_fileinfo.h i_utils.h i_common.h

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

libamidi_plug_la_LDFLAGS = $(PLUGIN_LDFLAGS)
libamidi_plug_la_LIBADD = $(GTK_LIBS) @ALSA_LIBS@ -lpthread
libamidi_plug_la_SOURCES = amidi-plug.c i_seq.c i_midi.c \
                           i_configure.c i_utils.c i_fileinfo.c \
                           amidi-plug.logo.xpm amidi-plug.midiicon.xpm

INCLUDES = $(GTK_CFLAGS) @ALSA_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)
