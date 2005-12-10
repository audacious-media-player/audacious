INCLUDES = -I../../..

noinst_LTLIBRARIES = libarchive.la
libarchive_la_LDFLAGS = -module -avoid-version
libarchive_la_SOURCES = archive.cpp     \
                        open.cpp        \
                        arch_raw.cpp    \
                        arch_gzip.cpp   \
                        arch_zip.cpp    \
                        arch_rar.cpp    \
                        arch_bz2.cpp

noinst_HEADERS        = archive.h       \
                        open.h          \
                        arch_raw.h      \
                        arch_gzip.h     \
                        arch_zip.h      \
                        arch_rar.h      \
                        arch_bz2.h
