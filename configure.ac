dnl ***
dnl *** Process this file with autoconf to produce a configure script.
dnl ***

dnl Initialize
dnl ==========
AC_PREREQ([2.59])
AC_INIT([audacious], [3.5])
AC_COPYRIGHT([(C) 2005-2014 Audacious Team])

AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE_NAME", [Name of package])
AC_DEFINE_UNQUOTED(VERSION, "$PACKAGE_VERSION", [Version number of package])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AUD_COMMON_PROGS

BUILDSYS_SHARED_LIB
AC_DEFINE_UNQUOTED(PLUGIN_SUFFIX, "$PLUGIN_SUFFIX", [Suffix for plugins])

m4_ifdef([PKG_INSTALLDIR], [PKG_INSTALLDIR], AC_SUBST([pkgconfigdir], ${libdir}/pkgconfig))

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

dnl Headers and functions
dnl ===============================

AC_CHECK_FUNCS([sigwait])

AC_MSG_CHECKING([for /proc/self/exe])
if test -e "/proc/self/exe" ; then
    AC_MSG_RESULT(yes)
    AC_DEFINE([HAVE_PROC_SELF_EXE], [1], [Define if the /proc/self/exe symlink is supported])
else
    AC_MSG_RESULT(no)
fi

dnl iconv
dnl =====

AM_ICONV
LIBS="$LIBS $LIBICONV"

dnl gettext
dnl =======
AM_GNU_GETTEXT([external])
if test "$MSGFMT" = ":" ; then
    AC_MSG_ERROR([msgfmt was not found; have you installed gettext?])
fi

LIBS="$LIBS $LIBINTL"

dnl Build stamp
dnl ===========
AC_ARG_WITH([buildstamp],
 AS_HELP_STRING([--with-buildstamp=XXX], [Set build stamp to XXX]),
 AC_DEFINE_UNQUOTED([BUILDSTAMP], "$withval", [Build stamp]),
 AC_DEFINE_UNQUOTED([BUILDSTAMP], "unknown build", [Build stamp]))

dnl Activation of text console (MS-Windows ONLY)
dnl ============================================
AC_ARG_ENABLE([text-console],
[AS_HELP_STRING([--enable-text-console],[Enable text console for MS-Windows hosts])],
[use_text_console=$withval],
[use_text_console=no]
)

if test "x$HAVE_MSWINDOWS" = "xyes" && test "x$use_text_console" = "xno" ; then
    LDFLAGS="$LDFLAGS -Wl,-subsystem,windows"
fi

dnl Character set detection
dnl =======================

AC_ARG_ENABLE(chardet,
 AS_HELP_STRING(--enable-chardet, [Disable character set detection (default=auto)]),
 USE_CHARDET=$enableval, USE_CHARDET=auto)

if test $USE_CHARDET != no ; then
    PKG_CHECK_MODULES(LIBGUESS, libguess >= 1.2, have_libguess=yes, have_libguess=no)
    if test $have_libguess = yes ; then
        USE_CHARDET=yes
        AC_DEFINE(USE_CHARDET, 1, [Define if character set detection enabled])
    elif test $USE_CHARDET = yes ; then
        AC_MSG_ERROR([Character set detection could not be enabled; check config.log])
    else
        USE_CHARDET=no
    fi
fi

AC_SUBST(USE_CHARDET)

dnl D-Bus support
dnl =============

AC_ARG_ENABLE(dbus,
 AS_HELP_STRING(--disable-dbus, [Disable D-Bus support (default=enabled)]),
 USE_DBUS=$enableval, USE_DBUS=yes)

if test $USE_DBUS = yes ; then
	AC_CHECK_PROG(have_dbus, gdbus-codegen, yes, no)
	if test $have_dbus = no ; then
		AC_MSG_ERROR([D-Bus support unavailable; use --disable-dbus for a crippled build])
	fi
    AC_DEFINE(USE_DBUS, 1, [Define if D-Bus support enabled])
fi

AC_SUBST(USE_DBUS)

dnl Valgrind analysis support
dnl =========================

AC_ARG_ENABLE(valgrind,
 AS_HELP_STRING(--enable-valgrind, [Valgrind analysis support (default=disabled)]),
 enable_valgrind=$enableval, enable_valgrind=no)

if test $enable_valgrind = yes ; then
    AC_DEFINE(VALGRIND_FRIENDLY, 1, [Define to allow Valgrind analysis])
    VALGRIND_FRIENDLY=1
else
    VALGRIND_FRIENDLY=0
fi

AC_SUBST([VALGRIND_FRIENDLY])

dnl Paths
dnl =====

plugindir="$libdir/audacious"
AC_SUBST([plugindir])

localedir="$datarootdir/locale"
AC_SUBST([localedir])

dnl Reliably #include "config.h" (for large file support)
dnl =====================================================
CPPFLAGS="$CPPFLAGS -include config.h"


### ---------------------------------------------------------------------------
dnl Output configuration files
dnl ==========================
AC_CONFIG_FILES([
    audacious.pc
    buildsys.mk
    extra.mk
    man/audtool.1
    man/audacious.1
    src/libaudcore/audio.h
    src/libaudcore/tinylock.h
])

AC_OUTPUT

### ---------------------------------------------------------------------------
dnl Print out the results
dnl =====================
echo ""
echo "Configuration:"
echo ""
echo "  Install path:                           $prefix"
echo ""
echo "  Automatic character code detection:     $USE_CHARDET"
echo "  D-Bus support:                          $USE_DBUS"
echo "  Valgrind analysis support:              $enable_valgrind"
echo ""
