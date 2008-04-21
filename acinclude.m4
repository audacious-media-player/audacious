
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
AC_DEFUN([AUD_CONDITIONAL], [AM_CONDITIONAL([$1],[dnl
test "x${$2}" = m4_ifval([$3], ["x$3"],["xyes"])dnl
])])


dnl ** Simple wrapper for AC_ARG_ENABLE
dnl ** AUD_ARG_ENABLE([name], [default value], [help string], [if enabled], [if disabled])
AC_DEFUN([AUD_ARG_ENABLE], [dnl
    define([Name], [translit([$1], [./-], [___])])
    AC_ARG_ENABLE([$1], [$3],, [enable_[]Name=$2])
    if test "x${enable_[]Name}" = "xyes"; then
        m4_ifvaln([$4], [$4], [:])dnl
        m4_ifvaln([$5], [else $5])dnl
    fi
])


AC_DEFUN([AUD_ARG_SIMPLE], [dnl
    define([Name], [translit([$1], [./-], [___])])
    AC_ARG_ENABLE([$1], [$3],, [enable_[]Name=$2])
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
    AC_CACHE_CHECK([for GNU make],aud_gnu_make_command,[
    aud_gnu_make_command=""
    for a in "$MAKE" make gmake gnumake; do
        test "x$a" = "x" && continue
        if ( sh -c "$a --version" 2>/dev/null | grep "GNU Make" >/dev/null ) ; then
            aud_gnu_make_command="$a"
            break
        fi
    done
    ])
    if test "x$aud_gnu_make_command" != "x" ; then
        MAKE="$aud_gnu_make_command"
    else
        AC_MSG_ERROR([** GNU make not found. If it is installed, try setting MAKE environment variable. **])
    fi
    AC_SUBST([MAKE])
])


dnl *** Define plugin directories
AC_DEFUN([AUD_DEFINE_PLUGIN_DIR],[
define([Name], [translit([$1], [a-z], [A-Z])])dnl
if test "x$enable_one_plugin_dir" = "xyes"; then
ifdef([aud_plugin_dirs_defined],[],
[    pluginsubs="\\\"Plugins\\\""
])dnl
    Name[]_PLUGIN_DIR="Plugins"
else
    ifdef([aud_plugin_dirs_defined],
    [pluginsubs="$pluginsubs,\\\"$1\\\""],
    [pluginsubs="\\\"$1\\\""])
    Name[]_PLUGIN_DIR="$1"
fi
AC_SUBST(Name[]_PLUGIN_DIR)dnl
define([aud_plugin_dirs_defined],[1])dnl
])dnl
