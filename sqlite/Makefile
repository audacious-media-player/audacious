include ../mk/rules.mk
include ../mk/init.mk

OBJECTIVE_LIBS_NOINST = libsqlite.a

CFLAGS+=-DNO_TCL=1

SOURCES=alter.c analyze.c attach.c auth.c btree.c build.c \
	callback.c complete.c date.c delete.c \
	expr.c func.c hash.c insert.c \
	main.c opcodes.c os.c os_os2.c os_unix.c os_win.c \
	pager.c parse.c pragma.c prepare.c printf.c random.c \
	select.c table.c tokenize.c trigger.c \
	update.c util.c vacuum.c \
	vdbe.c vdbeapi.c vdbeaux.c vdbefifo.c vdbemem.c \
	where.c utf.c legacy.c

OBJECTS=${SOURCES:.c=.o}

include ../mk/objective.mk

libsqlite.a: $(OBJECTS)
	$(AR) cq $@ $(OBJECTS)

.y.c:
	
