libdir = `xmms-config --input-plugin-dir`
lib_LTLIBRARIES = libmp4.la
libmp4_la_CFLAGS = -I$(top_srcdir)/libmp4v2 -I$(top_srcdir)/include `xmms-config --cflags` -DHAVE_GLIB_H=1
libmp4_la_LDFLAGS = -module -avoid-version `xmms-config --libs` -lpthread -lstdc++
libmp4_la_LIBADD = $(top_builddir)/libfaad2/libfaad.la $(top_builddir)/libmp4v2/libmp4v2.la
libmp4_la_SOURCES = libmp4.c mp4_utils.c aac_utils.c
