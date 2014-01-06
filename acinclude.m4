dnl Add $1 to CFLAGS and CXXFLAGS if supported
dnl ------------------------------------------

AC_DEFUN([AUD_CHECK_CFLAGS],[
    AC_MSG_CHECKING([whether the C/C++ compiler supports $1])
    OLDCFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS $1 -Werror"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[return 0;])],[
        AC_MSG_RESULT(yes)
        CFLAGS="$OLDCFLAGS $1"
        CXXFLAGS="$CXXFLAGS $1"
    ],[
        AC_MSG_RESULT(no)
        CFLAGS="$OLDCFLAGS"
    ])
])


dnl **
dnl ** Common checks
dnl **
AC_DEFUN([AUD_COMMON_PROGS], [

dnl Check for C and C++ compilers
dnl =============================
AC_REQUIRE([AC_PROG_CC])
AC_REQUIRE([AC_PROG_CXX])
AC_REQUIRE([AC_C_BIGENDIAN])
AC_REQUIRE([AC_SYS_LARGEFILE])

if test "x$GCC" = "xyes"; then
    CFLAGS="$CFLAGS -std=gnu99 -ffast-math -Wall -pipe"
    CXXFLAGS="$CXXFLAGS -Wall -pipe"
    AUD_CHECK_CFLAGS(-Wtype-limits)
fi

dnl Check platform
dnl ==============

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_MSG_CHECKING([operating system type])

HAVE_LINUX=no
HAVE_MSWINDOWS=no

case "$target" in
    *linux*)
        AC_MSG_RESULT(Linux)
        HAVE_LINUX=yes
        ;;
    *mingw*)
        AC_MSG_RESULT(Windows)
        HAVE_MSWINDOWS=yes
        ;;
    *)
        AC_MSG_RESULT(other UNIX)
        ;;
esac

AC_SUBST(HAVE_MSWINDOWS)
AC_SUBST(HAVE_LINUX)

dnl Enable "-Wl,-z,defs" only on Linux
dnl ==================================
if test $HAVE_LINUX = yes ; then
	LDFLAGS="$LDFLAGS -Wl,-z,defs"
fi

dnl MinGW needs -march=i686 for atomics
dnl ===================================
if test $HAVE_MSWINDOWS = yes ; then
	CFLAGS="$CFLAGS -march=i686"
fi

dnl Checks for various programs
dnl ===========================
AC_PROG_LN_S
AC_PATH_PROG([RM], [rm])
AC_PATH_PROG([MV], [mv])
AC_PATH_PROG([CP], [cp])
AC_PATH_PROG([AR], [ar])
AC_PATH_PROG([RANLIB], [ranlib])
AC_PATH_PROG([WINDRES], [windres])

dnl Check for POSIX threads
dnl =======================
AC_SEARCH_LIBS([pthread_create], [pthread])

dnl Check for GTK+ and pals
dnl =======================

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.32)
PKG_CHECK_MODULES(GMODULE, gmodule-2.0 >= 2.32)
PKG_CHECK_MODULES(GTK, gtk+-3.0 >= 3.4)

if test $HAVE_MSWINDOWS = yes ; then
    PKG_CHECK_MODULES(GIO, gio-2.0 >= 2.32)
else
	PKG_CHECK_MODULES(GIO, gio-2.0 >= 2.32 gio-unix-2.0 >= 2.32)
fi

AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)
AC_SUBST(GIO_CFLAGS)
AC_SUBST(GIO_LIBS)
AC_SUBST(GMODULE_CFLAGS)
AC_SUBST(GMODULE_LIBS)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

])
