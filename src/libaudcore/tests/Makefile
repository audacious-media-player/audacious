all: test test-mainloop

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
       stubs.cc

FLAGS = -I.. -I../.. -DEXPORT= -DPACKAGE=\"audacious\" -DICONV_CONST= \
        $(shell pkg-config --cflags --libs glib-2.0) \
        -std=c++11 -Wall -g -O0 -fno-elide-constructors \
        -fprofile-arcs -ftest-coverage -pthread

test: ${SRCS} test.cc
	g++ ${SRCS} test.cc ${FLAGS} -o test

test-mainloop: ${SRCS} test-mainloop.cc
	g++ ${SRCS} test-mainloop.cc ${FLAGS} -DUSE_QT -fPIC \
	$(shell pkg-config --cflags --libs Qt5Core) \
	-o test-mainloop

cov: all
	rm -f *.gcda
	./test
	./test-mainloop
	./test-mainloop --qt
	gcov --object-directory . ${SRCS} ${MAINLOOP_SRCS}

clean:
	rm -f test test-mainloop *.gcno *.gcda *.gcov
