include ../../../../mk/rules.mk
include ../../../../mk/init.mk

OBJECTIVE_LIBS_NOINST = libparanormal.a

OBJECT_CFLAGS   = $(GTK_CFLAGS)
OBJECT_LIBS     = $(GTK_LIBS)

CFLAGS += -fPIC -DPIC -Wall -g -I../../../.. $(OBJECT_CFLAGS) $(XML_CPPFLAGS) -I..
LIBADD   = -lm $(OBJECT_LIBS) $(XML_LIBS)

SOURCES =  \
	pnactuator.c        \
	pnactuatorfactory.c \
	pnactuatorlist.c    \
	pnaudiodata.c       \
	pnblur.c            \
	pnbooleanoption.c   \
	pnbuiltins.c        \
	pncontainer.c       \
	pncpu.c             \
	pndisplacement.c    \
	pndistortion.c      \
	pnerror.c           \
	pnflip.c            \
	pnfloatoption.c     \
	pnimage.c           \
	pnimagecontext.c    \
	pninit.c            \
	pnintegeroption.c   \
	pnlistoption.c      \
	pnobject.c          \
	pnoption.c          \
	pnrotozoom.c        \
	pnscope.c           \
	pnscript.c          \
	pnscriptparser.c    \
	pnstringoption.c    \
	pnsymboltable.c     \
	pntestactuator.c    \
	pnuserobject.c      \
	pnvis.c             

OBJECTS = ${SOURCES:.c=.o}

include ../../../../mk/objective.mk
