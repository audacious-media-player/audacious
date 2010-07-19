dnl ***
dnl *** Process this file with autoconf to produce a configure script.
dnl ***

dnl Require autoconf 2.59 or later
AC_PREREQ([2.59])

dnl Initialize
AC_INIT([audacious], [2.4-beta1], [bugs+audacious@atheme.org])
AC_COPYRIGHT([(C) 2005-2010 Audacious Development Team])
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AC_GNU_SOURCE

AC_CONFIG_HEADERS([config.h])
PKG_PROG_PKG_CONFIG

dnl Build flags
dnl ===========
LDFLAGS="$LDFLAGS -z defs"

dnl buildsystem bootstrap
dnl =====================
dnl XXX kludge for FreeBSD
if test -d "/usr/X11R6/include"; then
    CPPFLAGS="$CPPFLAGS -I/usr/X11R6/include"
fi

AC_SUBST([PACKAGE], [AC_PACKAGE_TARNAME])
AC_SUBST([VERSION], [AC_PACKAGE_VERSION])
AC_DEFINE_UNQUOTED([PACKAGE], "$PACKAGE", [Name of package])
AC_DEFINE_UNQUOTED([VERSION], "$VERSION", [Version number of package])

BUILDSYS_PROG_IMPLIB
BUILDSYS_SHARED_LIB
IMPLIB_LIBS=""
AC_SUBST([IMPLIB_LIBS])


dnl Build stamp
dnl ===========
AC_ARG_WITH([buildstamp],
 AS_HELP_STRING([--with-buildstamp=XXX], [Set build stamp to XXX]),
 AC_DEFINE_UNQUOTED([BUILDSTAMP], "$withval", [Build stamp]),
 AC_DEFINE_UNQUOTED([BUILDSTAMP], "UNSUPPORTED VERSION", [Build stamp]))


dnl Large file support
dnl ==================
CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64"

dnl Debug support
dnl ========================
AC_SUBST(DEBUG)
AUD_ARG_ENABLE([debug], [no],
[additional debugging output],
[
    AC_DEFINE([DEBUG], 1, [Define if debugging output is required])
])
AUD_CONDITIONAL([DEBUG], [enable_debug])


dnl Plugin stuff
dnl ========================
SHARED_SUFFIX="$PLUGIN_SUFFIX"
AC_SUBST([SHARED_SUFFIX])
AC_DEFINE_UNQUOTED([SHARED_SUFFIX], "${SHARED_SUFFIX}",
    [Define the shared module suffix extension on your platform.])

AC_MSG_CHECKING([if you are running Apple-GCC])
case "$target" in
    *-apple-*)
        AC_MSG_RESULT([yes, sorry you poor bastard])
        LDFLAGS="$LDFLAGS -framework CoreFoundation -framework CoreServices -framework Carbon"
        ;;
    *)
        AC_MSG_RESULT([no])
        ;;
esac


dnl GNU gettext
dnl ===========
AM_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.14.0])

dnl Checks common for core and plugins
dnl ==================================
AUD_COMMON_PROGS


dnl Check for headers and functions
dnl ===============================
AC_CHECK_HEADERS([unistd.h fcntl.h sys/ioctl.h fnmatch.h limits.h wchar.h fts.h execinfo.h sys/signalfd.h sys/errno.h])
AC_CHECK_FUNCS([mkdtemp getmntinfo statvfs strtoul lrintf signalfd lstat])


### ---------------------------------------------------------------------------
dnl Path settings
dnl =============
AC_PATH_PROG([AUDACIOUS_PATH], [audacious], [no])

AUD_ARG_ENABLE([user-plugin-dir], [yes],
[per-user plugin dir])

if test "x$enable_user_plugin_dir" = "xno"; then
    AC_DEFINE([DISABLE_USER_PLUGIN_DIR], 1, [Define to disable per user plugin directory])
fi

AUD_ARG_ENABLE([rpath], [yes], [disable rpath])

if test "x$enable_rpath" = "xyes"; then
    AUDLDFLAGS="-Wl,-rpath,${libdir}"
fi

AC_SUBST([AUDLDFLAGS])

### ---------------------------------------------------------------------------
dnl Chardet support
dnl ===============
AUD_ARG_ENABLE([chardet], [yes],
[charset detection support],
[
    PKG_CHECK_MODULES([LIBGUESS], [libguess], [enable_chardet=yes], [enable_chardet=no])

    if test "x$enable_chardet" = "xyes"; then
        AC_DEFINE([USE_CHARDET], 1, [Define if character set detection enabled])
    fi
])
AUD_CONDITIONAL([USE_CHARDET], [enable_chardet])


### ---------------------------------------------------------------------------
dnl D-Bus support
dnl =============
AUD_ARG_ENABLE([dbus], [yes],
[D-Bus support],
[
    PKG_CHECK_MODULES([DBUS], [dbus-1 >= 0.60 dbus-glib-1 >= 0.60 gthread-2.0],
        [enable_dbus=yes],
        [enable_dbus=no])

    AC_PATH_PROG([DBUS_BINDING_TOOL], [dbus-binding-tool], [no])
    AC_PATH_PROG([GLIB_GENMARSHAL], [glib-genmarshal], [no])
    if test "x$DBUS_BINDING_TOOL" = "xno" || test "x$GLIB_GENMARSHAL" = "xno" || test "x$enable_dbus" = "xno" ; then
        enable_dbus="no"
    else
        ADD_PC_REQUIRES([dbus-1 >= 0.60])
        ADD_PC_REQUIRES([dbus-glib-1 >= 0.60])
        AC_DEFINE([USE_DBUS], 1, [Define if D-Bus support enabled])
        AC_DEFINE([DBUS_SERVICES_DIR], "$datadir/dbus-1/services",
            [Location of D-Bus services directory])
        LIBAUDCLIENT_LDFLAGS="$AUDLDFLAGS -laudclient"
        AC_SUBST([DBUS_CFLAGS])
        AC_SUBST([DBUS_LIBS])
        AC_SUBST([DBUS_SERVICES_DIR])
        AC_SUBST([DBUS_BINDING_TOOL])
        AC_SUBST([LIBAUDCLIENT_LDFLAGS])
    fi
])
AUD_CONDITIONAL([USE_DBUS], [enable_dbus])


### ---------------------------------------------------------------------------
dnl regex support (gnu/oniguruma/pcre)
dnl ==================================
REGEX_LIBS=
REGEX_CFLAGS=
AC_ARG_WITH([regexlib],
[AS_HELP_STRING([--with-regexlib[=gnu/oniguruma/pcre]],[Use the chosen regex library (def: gnu)])],
[
case $withval in
  pcre)
    AUD_CHECK_MODULE([LIBPCRE], [libpcre], [>= 6.7], [PCRE])
    AC_DEFINE([USE_REGEX_PCRE], 1, [If this macro is defined, use PCRE regex library.])
    REGEX_LIBS="-lpcreposix $LIBPCRE_LIBS"
    REGEX_CFLAGS="$LIBPCRE_CFLAGS"
    ;;
  oniguruma)
    AC_CHECK_LIB([onig], [onig_new],
    [
        AC_DEFINE([USE_REGEX_ONIGURUMA], 1, [If this macro is defined, use Oniguruma regex library.])
        REGEX_LIBS="-lonig"
    ],[
        AC_MSG_ERROR([Cannot find Oniguruma regex library.])
    ])
    ;;
  *)
    AC_DEFINE([USE_REGEX_GNU], 1, [If this macro is defined, use GNU regex library.])
    ;;
esac
],[
    AC_DEFINE([USE_REGEX_GNU], 1, [If this macro is defined, use GNU regex library.])
])
AC_SUBST([REGEX_LIBS])
AC_SUBST([REGEX_CFLAGS])


### ---------------------------------------------------------------------------
dnl Session management support
dnl ==========================
AS_AC_EXPAND([desktop_file], "${datadir}/applications/audacious2.desktop")
AC_DEFINE_UNQUOTED([AUDACIOUS_DESKTOP_FILE], "${desktop_file}", [Location of the desktop file])

AUD_ARG_ENABLE([sm], [yes],
[session management],
[
    PKG_CHECK_MODULES([SM], [sm >= 1.0], [
        AC_DEFINE([USE_EGGSM], 1, [Define if session management should be used.])
        enable_sm="yes"
    ],[
        enable_sm="no"
    ])

    AC_SUBST([SM_CFLAGS])
    AC_SUBST([SM_LIBS])

    PKG_CHECK_MODULES([ICE], [ice >= 1.0],
     [enable_sm="${enable_sm}"],
     [enable_sm="no"])

    AC_SUBST([ICE_CFLAGS])
    AC_SUBST([ICE_LIBS])
])
AUD_CONDITIONAL([USE_EGGSM], [enable_sm])

### ---------------------------------------------------------------------------
dnl Ugly system checks
dnl ==================
AC_MSG_CHECKING([if __ELF__ is defined])
AC_EGREP_CPP([yes],
    [#ifdef __ELF__
        yes
     #endif
    ],
    [is_elf=yes],
    [is_elf=no]
)
AC_MSG_RESULT([$is_elf])

have_solaris=no
have_sun=no
ARCH_DEFINES=""
case $host in
    *-*-openbsd* | *-*-netbsd* | *-*-mirbsd*)
        have_sun=yes
        if test "x${is_elf}" = "xno"; then
            AC_DEFINE([SYMBOL_PREFIX], "_", [Define to symbol prefix, if any])
        fi
    ;;
    *-*-darwin*)
        AC_DEFINE([SYMBOL_PREFIX], "_", [Define to symbol prefix, if any])
    ;;
    *-hpux-*)
        ARCH_DEFINES="-DHPUX"
    ;;
    *-*-solaris* | *-*-sunos*)
        have_solaris=yes
    ;;
esac

arch_type=other
x86_asm=no
case $host in
    i386-*-* | i486-*-* | i586-*-* | i686-*-* | i86pc-*-*)
        arch_type=ix86
        case $host in
            *-*-netbsd* | *-*openbsd*)
            ;;
            *)
                ARCH_DEFINES="-DI386_ASSEM"
                x86_asm=yes
            ;;
        esac
    ;;
esac

AC_SUBST([ARCH_DEFINES])

dnl XXX delete ASAP -nenolod
AUD_CONDITIONAL([ARCH_X86], [arch_type], [ix86])
AUD_CONDITIONAL([USE_X86ASM], [x86_asm])
AUD_CONDITIONAL([HAVE_SOLARIS], [have_solaris])
AUD_CONDITIONAL([HAVE_SUN], [have_sun])

### ---------------------------------------------------------------------------
dnl Define plugin directories
dnl =========================
plugindir="$libdir/audacious"
AC_SUBST([plugindir])
AC_SUBST([pluginsubs])

AUD_DEFINE_PLUGIN_DIR([Transport])
AUD_DEFINE_PLUGIN_DIR([Input])
AUD_DEFINE_PLUGIN_DIR([Output])
AUD_DEFINE_PLUGIN_DIR([Effect])
AUD_DEFINE_PLUGIN_DIR([General])
AUD_DEFINE_PLUGIN_DIR([Visualization])
AUD_DEFINE_PLUGIN_DIR([Container])
AUD_DEFINE_PLUGIN_DIR([Discovery])


### ---------------------------------------------------------------------------
dnl XXX Work around some autoconf bugs.
if test "x$prefix" = "xNONE"; then
        prefix="${ac_default_prefix}"
fi

if test -z "$datadir"; then
	datadir="${prefix}/share"
fi

localedir="$datadir/locale"
AC_SUBST([localedir])

AUDACIOUS_DEFINES="$CFLAGS -DDATA_DIR=\\\"$datadir/audacious\\\" -DPLUGIN_DIR=\\\"$plugindir\\\" -DPLUGINSUBS=$pluginsubs -DLOCALEDIR=\\\"$localedir\\\""
AC_SUBST([AUDACIOUS_DEFINES])


### ---------------------------------------------------------------------------
dnl Output configuration files
dnl ==========================
AC_CONFIG_FILES([
    audacious.pc
    audclient.pc
    buildsys.mk
    extra.mk
    man/audtool2.1
    man/audacious2.1
])

BUILDSYS_TOUCH_DEPS
AC_OUTPUT

### ---------------------------------------------------------------------------
dnl Print out the results
dnl =====================
echo ""
echo "Configuration:"
echo ""
echo "  Install path:                           $prefix"

if test "x$AUDACIOUS_PATH" != "xno"; then
echo "  Current Audacious executable:           $AUDACIOUS_PATH"
fi

echo "  Allow user plugin dir:                  $enable_user_plugin_dir"
echo ""
echo "  Additional debugging output:            $enable_debug"
echo "  Automatic character code detection:     $enable_chardet"
echo "  D-Bus support:                          $enable_dbus"
echo "  Session management (eggsm)              $enable_sm"
echo ""
echo "  SSE2:                                   $enable_sse2"
echo "  AltiVec:                                $enable_altivec"
echo ""

if test "x$AUDACIOUS_PATH" != "xno" && test "x$prefix/bin/audacious" != "x$AUDACIOUS_PATH"; then
    audlocation=`dirname $AUDACIOUS_PATH`
    echo "**"
    echo "** NOTICE! Configure has discovered that you already have Audacious installed"
    echo "** and it does not match with the given --prefix. You have Audacious installed"
    echo "** in ${audlocation} and you chose ${prefix}/bin."
    echo "**"
    echo "** If you don't want two copies of Audacious installed, rerun configure with"
    echo "** the --prefix option set to the location of the old Audacious, or uninstall"
    echo "** the old Audacious."
    echo "**"
fi

if test "x$enable_dbus" != "xyes"; then
    echo "**"
    echo "** WARNING! DBUS support is disabled. This means that various features"
    echo "** the user might expect (such as remotely adding files to session via"
    echo "** commandline) will not work!"
    echo "**"
fi
