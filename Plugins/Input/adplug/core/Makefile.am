noinst_LTLIBRARIES = libadplugcore.la

libadplugcore_la_SOURCES = adplug.cpp emuopl.cpp fmopl.c diskopl.cpp debug.c \
debug.h fprovide.cpp player.cpp database.cpp hsc.cpp sng.cpp imf.cpp \
players.cpp protrack.cpp a2m.cpp adtrack.cpp amd.cpp bam.cpp d00.cpp dfm.cpp dmo.cpp \
hsp.cpp ksm.cpp mad.cpp mid.cpp mkj.cpp cff.cpp dtm.cpp fmc.cpp mtk.cpp rad.cpp raw.cpp \
sa2.cpp s3m.cpp xad.cpp flash.cpp bmf.cpp hybrid.cpp hyp.cpp psi.cpp rat.cpp u6m.cpp \
rol.cpp xsm.cpp adlibemu.c dro.cpp lds.cpp mididata.h realopl.cpp analopl.cpp \
temuopl.cpp msc.cpp rix.cpp

# This is a hack. Throughout AdPlug, stricmp() is used to do caseless string
# comparations. UNIX libc's don't support stricmp(), but do support the BSD
# strcasecmp(), so we have to replace every occurance of stricmp() into
# strcasecmp(). stricmp() seems to be Windows, but maybe also POSIX...
AM_CPPFLAGS = -Dstricmp=strcasecmp

noinst_HEADERS = adplug.h emuopl.h fmopl.h silentopl.h opl.h diskopl.h \
a2m.h amd.h bam.h d00.h dfm.h hsc.h hsp.h imf.h ksm.h lds.h mid.h mkj.h mtk.h \
protrack.h rad.h raw.h sa2.h sng.h u6m.h player.h fmc.h mad.h xad.h bmf.h \
flash.h hyp.h psi.h rat.h hybrid.h rol.h adtrack.h cff.h dtm.h fprovide.h \
database.h players.h xsm.h adlibemu.h kemuopl.h dro.h dmo.h s3m.h realopl.h \
analopl.h temuopl.h msc.h rix.h

INCLUDES = $(BINIO_CFLAGS) -I$(top_builddir)/intl -I$(top_srcdir)
