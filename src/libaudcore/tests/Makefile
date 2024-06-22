all: test

SRCS = ../audio.cc \
       ../audstrings.cc \
       ../charset.cc \
       ../hook.cc \
       ../index.cc \
       ../logger.cc \
       ../mainloop.cc \
       ../multihash.cc \
       ../ringbuf.cc \
       ../stringbuf.cc \
       ../strpool.cc \
       ../tinylock.cc \
       ../threads.cc \
       ../tuple.cc \
       ../tuple-compiler.cc \
       ../util.cc \
       stubs.cc \
       test.cc \
       test-mainloop.cc

QT_DEP = Qt6Core
ifneq ($(shell pkg-config --exists ${QT_DEP} && echo 1),1)
	QT_DEP = Qt5Core
endif

FLAGS = -I.. -I../.. -DEXPORT= -DPACKAGE=\"audacious\" -DICONV_CONST= \
        $(shell pkg-config --cflags --libs glib-2.0 ${QT_DEP}) \
        -std=c++17 -Wall -g -O0 -fno-elide-constructors \
        -fprofile-arcs -ftest-coverage -pthread -dumpbase ''

test: ${SRCS}
	g++ ${SRCS} ${FLAGS} -DUSE_QT -fPIC -o test

cov: all
	rm -f *.gcda
	./test
	./test --qt
	gcov --object-directory . ${SRCS} ${MAINLOOP_SRCS}

clean:
	rm -f test *.gcno *.gcda *.gcov
