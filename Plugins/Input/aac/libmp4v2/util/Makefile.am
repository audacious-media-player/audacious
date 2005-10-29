
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib/mp4v2 -I$(top_builddir)/lib/mp4av

AM_CXXFLAGS = @BILLS_CPPWARNINGS@

bin_PROGRAMS = mp4dump mp4extract mp4info mp4trackdump mp4tags mp4art mp4videoinfo

mp4dump_SOURCES = mp4dump.cpp
mp4dump_LDADD = $(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la

mp4extract_SOURCES = mp4extract.cpp
mp4extract_LDADD = $(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la

mp4info_SOURCES = mp4info.cpp
mp4info_LDADD = $(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la

mp4trackdump_SOURCES = mp4trackdump.cpp
mp4trackdump_LDADD = $(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la

mp4videoinfo_SOURCES = mp4videoinfo.cpp
mp4videoinfo_LDADD = $(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la

mp4tags_SOURCES = mp4tags.cpp
mp4tags_LDADD = $(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la

mp4art_SOURCES = mp4art.cpp
mp4art_LDADD = $(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la

EXTRA_DIST = mp4dump60.dsp mp4extract60.dsp \
	mp4info.dsp 
