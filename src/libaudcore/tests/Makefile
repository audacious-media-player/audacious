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
       ../tuple.cc \
       ../tuple-compiler.cc \
       ../util.cc \
       stubs.cc \
       test.cc \
       test-mainloop.cc

FLAGS = -I.. -I../.. -DEXPORT= -DPACKAGE=\"audacious\" -DICONV_CONST= \
        $(shell pkg-config --cflags --libs glib-2.0) \
        -std=c++11 -Wall -g -O0 -fno-elide-constructors \
        -fprofile-arcs -ftest-coverage -pthread

test: ${SRCS}
	g++ ${SRCS} ${FLAGS} -DUSE_QT -fPIC \
	$(shell pkg-config --cflags --libs Qt5Core) \
	-o test

cov: all
	rm -f *.gcda
	./test
	./test --qt
	gcov --object-directory . ${SRCS} ${MAINLOOP_SRCS}

clean:
	rm -f test *.gcno *.gcda *.gcov
