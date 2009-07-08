
dnl ** ADD_PKG_REQUIRES([requirement])
dnl ** Adds a dependency to package's pkg-config file.
AC_DEFUN([ADD_PC_REQUIRES], [
   if test "x$PC_REQUIRES" = "x"; then
       PC_REQUIRES="$1"
   else
       PC_REQUIRES="$PC_REQUIRES, $1"
   fi
   AC_SUBST([PC_REQUIRES])
])


dnl ** AUD_CHECK_MODULE([define name], [module], [version required],
dnl **     [informational name], [additional error message])
dnl **
dnl ** Works like PKG_CHECK_MODULES, but provides an informative
dnl ** error message if the package is not found. NOTICE! Unlike
dnl ** PKG_C_M, this macro ONLY supports one module name!
dnl **
dnl ** AUD_CHECK_MODULE([GLIB], [gtk+-2.0], [>= 2.8.0], [Gtk+2], [See http://www.gtk.org/])
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


dnl ** AUD_CONDITIONAL([symbol], [variable to test][, value])
dnl ** Simplifying wrapper for AM_CONDITIONAL.
dnl **
dnl ** AUD_CONDITIONAL([FOO], [foo])
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
define([Name], [translit([$1], [./-], [___])])dnl
define([cBasce], [ifelse([$2],[yes],[Disable],[Enable]) $3 (def: ifelse([$2],[yes],[enabled],[disabled]))])dnl
    AC_ARG_ENABLE([$1], [AS_HELP_STRING([ifelse([$2],[yes],[--disable-$1],[--enable-$1])], cBasce)],, [enable_[]Name=$2])
    if test "x${enable_[]Name}" = "xyes"; then
        m4_ifvaln([$4], [$4], [:])dnl
        m4_ifvaln([$5], [else $5])dnl
    fi
])


AC_DEFUN([AUD_ARG_SIMPLE], [dnl
define([Name], [translit([$1], [./-], [___])])dnl
define([cBasce], [ifelse([$2],[yes],[Disable],[Enable]) $3 (def: ifelse([$2],[yes],[enabled],[disabled]))])dnl
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
    AC_CACHE_CHECK([for GNU make],_cv_gnu_make_command,[
    _cv_gnu_make_command=""
    for a in "$MAKE" make gmake gnumake; do
        test "x$a" = "x" && continue
        if ( sh -c "$a --version" 2>/dev/null | grep "GNU Make" >/dev/null ) ; then
            _cv_gnu_make_command="$a"
            break
        fi
    done
    ])
    if test "x$_cv_gnu_make_command" != "x" ; then
        MAKE="$_cv_gnu_make_command"
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



dnl **
dnl ** Common checks
dnl **
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
AC_PATH_PROG([TR], [tr])
AC_PATH_PROG([RANLIB], [ranlib])


dnl Check for Gtk+/GLib and pals
dnl ============================
AUD_CHECK_MODULE([GLIB], [glib-2.0], [>= 2.12.0], [Glib2])
AUD_CHECK_MODULE([GTHREAD], [gthread-2.0], [>= 2.12.0], [gthread-2.0])
AUD_CHECK_MODULE([GTK], [gtk+-2.0], [>= 2.8.0], [Gtk+2])
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
AUD_ARG_ENABLE([sse2], [yes], [SSE2 support],
[
    AC_MSG_CHECKING([SSE2 support])
    aud_my_save_CFLAGS="$CFLAGS"
    CFLAGS="-msse2"
    AC_TRY_RUN([
#include <emmintrin.h>
int main()
{
  _mm_setzero_pd();
  asm volatile("xorpd %xmm0,%xmm0\n\t");
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
AUD_ARG_ENABLE([altivec], [yes], [AltiVec support],
[
    AC_CHECK_HEADERS([altivec.h],
    [
        AC_DEFINE([HAVE_ALTIVEC], 1, [Define to 1 if your system has AltiVec.])
        AC_DEFINE([HAVE_ALTIVEC_H], 1, [Define to 1 if your system has an altivec.h file.])
        AC_DEFINE([ARCH_POWERPC], 1, [Define to 1 if your system is a PowerPC.])
        case $target in
             *-apple-*)
             SIMD_CFLAGS="-mpim-altivec"
             ;;
             *)
             SIMD_CFLAGS="-maltivec"
             ;;
        esac
        AC_SUBST([SIMD_CFLAGS])
    ],[
        enable_altivec="no"
    ])
])

])


dnl **
dnl ** Plugin helper macros
dnl **

dnl ** Unconditionally add a plugin to "build these" list
AC_DEFUN([AUD_PLUGIN_ADD], [dnl
define([Name], [translit([$1], [A-Z./-], [a-z___])])dnl
have_[]Name="yes"; res_short_[]Name="$1"
res_desc_[]Name="$3"; ifdef([aud_def_plugin_$2], [$2[]_PLUGINS="${$2[]_PLUGINS} $1"], [$2[]_PLUGINS="$1"])dnl
define([aud_def_plugin_$2],[1])dnl
])


dnl ** Generic template for macros below
AC_DEFUN([AUD_PLUGIN_CHK], [dnl
define([cBasce], [ifelse([$3],[yes],[Disable],[Enable]) $5 (def: ifelse([$3],[yes],[enabled],[disabled]))])dnl
AC_ARG_ENABLE([$1], [AS_HELP_STRING([ifelse([$3],[yes],[--disable-$1],[--enable-$1])], cBasce)],, [enable_$2="$3"])dnl
    have_$2="no"
    if test "x${enable_$2}" = "xyes"; then
        m4_ifvaln([$6], [$6], [:])
        if test "x${have_$2}" = "xyes"; then
            m4_ifvaln([$7], [$7], [:])dnl
        else
            res_msg_$2="(not found)"
            m4_ifvaln([$8], [$8], [:])dnl
        fi
    else
        res_msg_$2="(disabled)"
        m4_ifvaln([$9], [$9], [:])dnl
    fi
])


dnl ** Add a plugin based on --enable/--disable options
AC_DEFUN([AUD_PLUGIN_CHECK_SIMPLE], [dnl
define([cBasce], [ifelse([$2],[yes],[Disable],[Enable]) $6 (def: ifelse([$2],[yes],[enabled],[disabled]))])dnl
AC_ARG_ENABLE([$1], [AS_HELP_STRING([ifelse([$2],[yes],[--disable-$1],[--enable-$1])], cBasce)],, [enable_$2="$3"])dnl
    have_$2="no"
    if test "x${enable_$2}" = "xyes"; then
        m4_ifvaln([$6], [$6], [:])
    else
        res_msg_$2="(disabled)"
        m4_ifvaln([$7], [$7], [:])dnl
    fi
])


dnl ** Check and enable a plugin with a pkg-config check
AC_DEFUN([AUD_PLUGIN_CHECK_PKG], [dnl
define([Name], [translit([$1], [A-Z./-], [a-z___])])dnl
define([BigN], [translit([$1], [a-z./-], [A-Z___])])dnl
    AUD_PLUGIN_CHK([$1], Name, [$2], [$4], [$6], [dnl
        PKG_CHECK_MODULES([]BigN, [$7], [have_[]Name[]="yes"], [have_[]Name[]="no"])
    ], [
    AUD_PLUGIN_ADD([$5], [$3])
    m4_ifvaln([$8], [$8])
    ], [$9], [$10])
])


dnl ** Check and enable a plugin with a header files check
AC_DEFUN([AUD_PLUGIN_CHECK_HEADERS], [
define([Name], [translit([$1], [A-Z./-], [a-z___])])dnl
    AUD_PLUGIN_CHK([$1], Name, [$2], [$4], [$6], [
        AC_CHECK_HEADERS([$7], [have_[]Name[]="yes"], [have_[]Name[]="no"])
    ], [
    AUD_PLUGIN_ADD([$5], [$3])
    m4_ifvaln([$8], [$8])
    ], [$9], [$10])
])


dnl ** Check and enable a plugin with a complex check
AC_DEFUN([AUD_PLUGIN_CHECK_COMPLEX], [
# CHECK_COMPLEX #1 : $1
define([Name], [translit([$1], [A-Z./-], [a-z___])])dnl
    AUD_PLUGIN_CHK([$1], Name, [$2], [$4], [$6], [
# CHECK_COMPLEX #2 BEGIN
        $7
# CHECK_COMPLEX #2 END
    ], [
# CHECK_COMPLEX #3 BEGIN
    AUD_PLUGIN_ADD([$5], [$3])
    m4_ifvaln([$8], [$8])
# CHECK_COMPLEX #3 END
    ], [$9], [$10])
# CHECK_COMPLEX #4 END
])
