
dnl ** ADD_PKG_REQUIRES([requirement])
AC_DEFUN([ADD_PC_REQUIRES], [
   if test "x$PC_REQUIRES" = "x"; then
       PC_REQUIRES="$1"
   else
       PC_REQUIRES="$PC_REQUIRES, $1"
   fi
   AC_SUBST([PC_REQUIRES])
])


dnl ** Like PKG_CHECK_MODULES, but provides an informative error message.
dnl ** AUD_CHECK_MODULE([define name], [module], [version required],
dnl **     [informational name], [additional error message])
dnl **
dnl ** AUD_CHECK_MODULE([GLIB], [gtk+-2.0], [>= 2.10.0], [Gtk+2], [See http://www.gtk.org/])
AC_DEFUN([AUD_CHECK_MODULE], [
    PKG_CHECK_MODULES([$1], [$2 $3], [
        ADD_PC_REQUIRES([$2 $3])
    ],[
        PKG_CHECK_EXISTS([$2], [
            cv_pkg_version=`$PKG_CONFIG --modversion "$2" 2>/dev/null`
            AC_MSG_ERROR([[
$4 version $cv_pkg_version was found, but $2 $3 is required.
$5]])
        ],[
            AC_MSG_ERROR([[
Cannot find $4! If you are using binary packages based system, check that you
have the corresponding -dev/devel packages installed.
$5]])
        ])
    ])
])


dnl ** Simplifying wrapper
AC_DEFUN([AUD_CONDITIONAL],
[if test "x${$2}" = m4_ifval([$3], ["x$3"],["xyes"]) ; then
    $1="yes"
else
    $1="no"
fi
AC_SUBST([$1])dnl
])


dnl ** Simple wrapper for AC_ARG_ENABLE
dnl ** AUD_ARG_ENABLE([name], [default value], [help string], [if enabled], [if disabled])
AC_DEFUN([AUD_ARG_ENABLE], [dnl
# _A_ARG_ENABLE($1, $2, $3, $4, $5)
    define([Name], [translit([$1], [./-], [___])])dnl
    define([cBasce], [$3 (def: ifelse([$2],[yes],[enabled],[disabled]))])dnl
    AC_ARG_ENABLE([$1], [AS_HELP_STRING([ifelse([$2],[yes],[--disable-$1],[--enable-$1])], cBasce)],, [enable_[]Name=$2])
    if test "x${enable_[]Name}" = "xyes"; then
        m4_ifvaln([$4], [$4], [:])dnl
        m4_ifvaln([$5], [else $5])dnl
    fi
])


AC_DEFUN([AUD_ARG_SIMPLE], [dnl
# _A_ARG_SIMPLE($1, $2, $3, $4, $5, $6)
    define([Name], [translit([$1], [./-], [___])])dnl
    define([cBasce], [$3 (def: ifelse([$2],[yes],[enabled],[disabled]))])dnl
    AC_ARG_ENABLE([$1], [AS_HELP_STRING([ifelse([$2],[yes],[--disable-$1],[--enable-$1])], cBasce)],, [enable_[]Name=$2])
    if test "x${enable_[]Name}" = "xyes"; then
        AC_DEFINE([$4], [$5], [$6])
    fi
    AUD_CONDITIONAL([$4], [enable_$1])
    AC_SUBST([$4])
])


dnl ** Wrapper for cached compilation check
dnl ** AUD_TRY_COMPILE([message], [result variable], [includes], [body], [if ok], [if not ok])
AC_DEFUN([AUD_TRY_COMPILE], [dnl
    AC_CACHE_CHECK([$1], [$2],
    [AC_TRY_COMPILE([$3], [$4], [$2="yes"], [$2="no"])])
    if test "x${$2}" = "xyes"; then
        m4_ifvaln([$5], [$5], [:])dnl
        m4_ifvaln([$6], [else $6])dnl
    fi
])


dnl ** Check for GNU make
AC_DEFUN([AUD_CHECK_GNU_MAKE],[
    AC_CACHE_CHECK([for GNU make],cv_gnu_make_command,[
    cv_gnu_make_command=""
    for a in "$MAKE" make gmake gnumake; do
        test "x$a" = "x" && continue
        if ( sh -c "$a --version" 2>/dev/null | grep "GNU Make" >/dev/null ) ; then
            cv_gnu_make_command="$a"
            break
        fi
    done
    ])
    if test "x$cv_gnu_make_command" != "x" ; then
        MAKE="$cv_gnu_make_command"
    else
        AC_MSG_ERROR([** GNU make not found. If it is installed, try setting MAKE environment variable. **])
    fi
    AC_SUBST([MAKE])dnl
])dnl


dnl *** Define plugin directories
AC_DEFUN([AUD_DEFINE_PLUGIN_DIR],[dnl
define([Name], [translit([$1], [a-z], [A-Z])])dnl
if test "x$enable_one_plugin_dir" = "xyes"; then
ifdef([aud_plugin_dirs_defined],[],
[    pluginsubs="\\\"Plugins\\\""
])dnl
    Name[]_PLUGIN_DIR="Plugins"
else
    ifdef([aud_def_plugin_dirs_defined],
    [pluginsubs="$pluginsubs,\\\"$1\\\""],
    [pluginsubs="\\\"$1\\\""])
    Name[]_PLUGIN_DIR="$1"
fi
AC_SUBST(Name[]_PLUGIN_DIR)dnl
define([aud_def_plugin_dirs_defined],[1])dnl
])dnl


dnl *** Get plugin directories
AC_DEFUN([AUD_GET_PLUGIN_DIR],[dnl
define([Name], [translit([$1_plugin_dir], [A-Z], [a-z])])dnl
define([BigName], [translit([$1], [a-z], [A-Z])])dnl
ifdef([aud_get_plugin_dirs_defined],
[pluginsubs="$pluginsubs,\\\"$1\\\""],
[pluginsubs="\\\"$1\\\""])
BigName[]_PLUGIN_DIR=`pkg-config audacious --variable=[]Name[]`
AC_SUBST(BigName[]_PLUGIN_DIR)dnl
define([aud_get_plugin_dirs_defined],[1])dnl
])dnl



dnl ***
dnl *** Common checks
dnl ***
AC_DEFUN([AUD_COMMON_PROGS], [

dnl Check for C and C++ compilers
dnl =============================
AUD_CHECK_GNU_MAKE
AC_PROG_CC
AC_PROG_CXX
AM_PROG_AS
AC_ISC_POSIX
AC_C_BIGENDIAN

if test "x$GCC" = "xyes"; then
    CFLAGS="$CFLAGS -Wall -pipe"
    CXXFLAGS="$CXXFLAGS -pipe -Wall"
fi

dnl Checks for various programs
dnl ===========================
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PATH_PROG([RM], [rm])
AC_PATH_PROG([MV], [mv])
AC_PATH_PROG([CP], [cp])
AC_PATH_PROG([AR], [ar])
AC_PATH_PROG([RANLIB], [ranlib])


dnl Check for Gtk+/GLib and pals
dnl ============================
AUD_CHECK_MODULE([GLIB], [glib-2.0], [>= 2.14.0], [Glib2])
AUD_CHECK_MODULE([GTHREAD], [gthread-2.0], [>= 2.14.0], [gthread-2.0])
AUD_CHECK_MODULE([GTK], [gtk+-2.0], [>= 2.10.0], [Gtk+2])
AUD_CHECK_MODULE([PANGO], [pango], [>= 1.8.0], [Pango])
AUD_CHECK_MODULE([CAIRO], [cairo], [>= 1.2.4], [Cairo])


dnl Check for libmowgli
dnl ===================
AUD_CHECK_MODULE([MOWGLI], [libmowgli], [>= 0.4.0], [libmowgli],
    [http://www.atheme.org/projects/mowgli.shtml])


dnl Check for libmcs
dnl ================
AUD_CHECK_MODULE([LIBMCS], [libmcs], [>= 0.7], [libmcs],
    [http://www.atheme.org/projects/mcs.shtml])


dnl SSE2 support
dnl ============
AUD_ARG_ENABLE([sse2], [yes],
[Disable SSE2 support],
[
    AC_MSG_CHECKING([SSE2 support])
    aud_my_save_CFLAGS="$CFLAGS"
    CFLAGS="-msse2"
    AC_TRY_RUN([
#include <emmintrin.h>
int main()
{
  _mm_setzero_pd();
  return 0;
}
    ],[
        AC_MSG_RESULT([yes])
        AC_DEFINE([HAVE_SSE2], 1, [Define to 1 if your system has SSE2 support])
        SIMD_CFLAGS="-msse2"
    ],[
        AC_MSG_RESULT([no])
        enable_sse2="no"
    ])
    AC_SUBST([SIMD_CFLAGS])
    CFLAGS="$aud_my_save_CFLAGS"
])

dnl AltiVec support 
dnl ===============
AUD_ARG_ENABLE([altivec], [yes],
[Disable AltiVec support],
[
    AC_CHECK_HEADERS([altivec.h],
    [
        AC_DEFINE([HAVE_ALTIVEC], 1, [Define to 1 if your system has AltiVec.])
        AC_DEFINE([HAVE_ALTIVEC_H], 1, [Define to 1 if your system has an altivec.h file.])
        AC_DEFINE([ARCH_POWERPC], 1, [Define to 1 if your system is a PowerPC.])
        SIMD_CFLAGS="-maltivec"
        AC_SUBST([SIMD_CFLAGS])
    ],[
        enable_altivec="no"
    ])
])    

])
