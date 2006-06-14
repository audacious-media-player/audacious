include ../../../../mk/rules.mk
include ../../../../mk/objective.mk

OBJECTIVE_LIBS_NOINST = libjma.a

SOURCES = \
	7zlzma.cpp		\
	crc32.cpp		\
	iiostrm.cpp		\
	inbyte.cpp		\
	jma.cpp			\
	lzma.cpp		\
	lzmadec.cpp		\
	winout.cpp

OBJECTS = ${SOURCES:.cpp=.o}

LIBADD += -lstdc++
CXXFLAGS += -fPIC -DPIC $(GTK_CFLAGS) $(ARCH_DEFINES) -I../../../intl -I../../..
