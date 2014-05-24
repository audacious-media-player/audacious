SRCS = ../audstrings.cc \
       ../charset.cc \
       ../hook.cc \
       ../index.cc \
       ../multihash.cc \
       ../stringbuf.cc \
       ../strpool.cc \
       ../tinylock.cc \
       ../tuple.cc \
       ../tuple_compiler.cc \
       ../tuple_formatter.cc \
       test.cc

FLAGS = -I.. -DEXPORT= -DPACKAGE=\"audacious\" \
        $(shell pkg-config --cflags --libs glib-2.0) \
        -std=c++11 -Wall -g -O0 -fno-elide-constructors \
        -fprofile-arcs -ftest-coverage -pthread

test: ${SRCS}
	g++ ${SRCS} ${FLAGS} -o test

cov: test
	rm -f *.gcda
	./test
	gcov --object-directory . ${SRCS}

clean:
	rm -f test *.gcno *.gcda *.gcov
