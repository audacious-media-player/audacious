dnl Backward compatibility with older pkg-config <= 0.28
dnl Retrieves the value of the pkg-config variable
dnl for the given module.
dnl PKG_CHECK_VAR(VARIABLE, MODULE, CONFIG-VARIABLE,
dnl [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl ====================================================
m4_ifndef([PKG_CHECK_VAR], [
AC_DEFUN([PKG_CHECK_VAR],
[AC_REQUIRE([PKG_PROG_PKG_CONFIG])dnl
AC_ARG_VAR([$1], [value of $3 for $2, overriding pkg-config])dnl

_PKG_CONFIG([$1], [variable="][$3]["], [$2])
AS_VAR_COPY([$1], [pkg_cv_][$1])

AS_VAR_IF([$1], [""], [$5], [$4])dnl
])
])


dnl Add $1 to CFLAGS and CXXFLAGS if supported
dnl ==========================================

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

dnl Add $1 to CXXFLAGS if supported
dnl ===============================

AC_DEFUN([AUD_CHECK_CXXFLAGS],[
    AC_MSG_CHECKING([whether the C++ compiler supports $1])
    AC_LANG_PUSH([C++])
    OLDCXXFLAGS="$CXXFLAGS"
    CXXFLAGS="$CXXFLAGS $1 -Werror"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[return 0;])],[
        AC_MSG_RESULT(yes)
        CXXFLAGS="$OLDCXXFLAGS $1"
    ],[
        AC_MSG_RESULT(no)
        CXXFLAGS="$OLDCXXFLAGS"
    ])
    AC_LANG_POP([C++])
])


dnl **
dnl ** Common checks
dnl **
AC_DEFUN([AUD_COMMON_PROGS], [

dnl Check platform
dnl ==============

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_MSG_CHECKING([operating system type])

HAVE_LINUX=no
HAVE_MSWINDOWS=no
HAVE_DARWIN=no

case "$target" in
    *linux*)
        AC_MSG_RESULT(Linux)
        HAVE_LINUX=yes
        ;;
    *mingw*)
        AC_MSG_RESULT(Windows)
        HAVE_MSWINDOWS=yes
        ;;
    *darwin*)
        AC_MSG_RESULT(Darwin)
        HAVE_DARWIN=yes
        ;;
    *)
        AC_MSG_RESULT(other UNIX)
        ;;
esac

AC_SUBST(HAVE_MSWINDOWS)
AC_SUBST(HAVE_LINUX)
AC_SUBST(HAVE_DARWIN)

dnl Check for C and C++ compilers
dnl =============================
AC_REQUIRE([AC_PROG_CC])
AC_REQUIRE([AC_PROG_CXX])
AC_REQUIRE([AC_C_BIGENDIAN])
AC_REQUIRE([AC_SYS_LARGEFILE])

if test "x$GCC" = "xyes"; then
    CFLAGS="$CFLAGS -std=gnu99 -ffast-math -Wall -pipe"
    if test "x$HAVE_DARWIN" = "xyes"; then
        CXXFLAGS="$CXXFLAGS -std=gnu++11 -ffast-math -Wall -pipe"
        LDFLAGS="$LDFLAGS"
    else
        CXXFLAGS="$CXXFLAGS -std=gnu++11 -ffast-math -Wall -pipe"
    fi
    AUD_CHECK_CFLAGS(-Wtype-limits)
    AUD_CHECK_CFLAGS(-Wno-stringop-truncation)
    AUD_CHECK_CXXFLAGS(-Woverloaded-virtual)
fi

dnl On Mac, check for Objective-C and -C++ compilers
dnl ================================================

if test "x$HAVE_DARWIN" = "xyes"; then
    AC_PROG_OBJC
    AC_PROG_OBJCPP
    AC_PROG_OBJCXX
    AC_PROG_OBJCXXCPP

    OBJCXXFLAGS="$OBJCXXFLAGS -std=c++11"
fi

dnl Enable "-Wl,-z,defs" only on Linux
dnl ==================================
if test $HAVE_LINUX = yes ; then
    LDFLAGS="$LDFLAGS -Wl,-z,defs"
fi

dnl MinGW-specific flags
dnl ====================
if test $HAVE_MSWINDOWS = yes ; then
    AC_DEFINE([__USE_MINGW_ANSI_STDIO], [1], "Use GNU-style printf")
    CFLAGS="$CFLAGS -march=i686"
fi

dnl Byte order
dnl ==========
AC_C_BIGENDIAN([BIGENDIAN=1], [BIGENDIAN=0],
 [AC_MSG_ERROR([Unknown machine byte order])],
 [AC_MSG_ERROR([Universal builds are not supported, sorry])])
AC_SUBST([BIGENDIAN])

dnl Prevent symbol collisions
dnl =========================
if test "x$HAVE_MSWINDOWS" = "xyes" ; then
    EXPORT="__declspec(dllexport)"
elif test "x$GCC" = "xyes" ; then
    CFLAGS="$CFLAGS -fvisibility=hidden"
    CXXFLAGS="$CXXFLAGS -fvisibility=hidden"
    EXPORT="__attribute__((visibility(\"default\")))"
else
    AC_MSG_ERROR([Unknown syntax for EXPORT keyword])
fi
AC_DEFINE_UNQUOTED([EXPORT], [$EXPORT], [Define to compiler syntax for public symbols])

dnl Checks for various programs
dnl ===========================
AC_PROG_LN_S
AC_PATH_PROG([RM], [rm])
AC_PATH_PROG([MV], [mv])
AC_PATH_PROG([CP], [cp])
AC_PATH_TOOL([AR], [ar])
AC_PATH_TOOL([RANLIB], [ranlib])
AC_PATH_TOOL([WINDRES], [windres])

dnl Check for POSIX threads
dnl =======================
AC_SEARCH_LIBS([pthread_create], [pthread])

dnl Check for GTK+ and pals
dnl =======================

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.32)
PKG_CHECK_MODULES(GMODULE, gmodule-2.0 >= 2.32)

dnl GTK+ support
dnl =============

AC_ARG_ENABLE(gtk,
 AS_HELP_STRING(--disable-gtk, [Disable GTK+ support (default=enabled)]),
 USE_GTK=$enableval, USE_GTK=yes)

if test $USE_GTK = yes ; then
    PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.24)
    AC_DEFINE(USE_GTK, 1, [Define if GTK+ support enabled])
fi

AC_SUBST(USE_GTK)

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

dnl Qt support
dnl ==========

AC_ARG_ENABLE(qt,
 AS_HELP_STRING(--enable-qt, [Enable Qt support (default=disabled)]),
 USE_QT=$enableval, USE_QT=no)

if test $USE_QT = yes ; then
    PKG_CHECK_MODULES([QTCORE], [Qt5Core >= 5.2])
    PKG_CHECK_VAR([QTBINPATH], [Qt5Core >= 5.2], [host_bins])
    PKG_CHECK_MODULES([QT], [Qt5Core Qt5Gui Qt5Widgets >= 5.2])
    AC_DEFINE(USE_QT, 1, [Define if Qt support enabled])

    # needed if Qt was built with -reduce-relocations
    QTCORE_CFLAGS="$QTCORE_CFLAGS -fPIC"
    QT_CFLAGS="$QT_CFLAGS -fPIC"
fi

AC_SUBST(USE_QT)
AC_SUBST(QT_CFLAGS)
AC_SUBST(QT_LIBS)
AC_SUBST(QTBINPATH)

])
