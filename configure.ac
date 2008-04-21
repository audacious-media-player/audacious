dnl ***
dnl *** Process this file with autoconf to produce a configure script.
dnl ***

dnl Require autoconf 2.59 or later
AC_PREREQ([2.59])

dnl Initialize
AC_INIT([audacious], [1.9.0], [bugs+audacious@atheme.org])
AC_COPYRIGHT([(C) 2005-2008 Audacious Development Team])
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AC_GNU_SOURCE
AC_CONFIG_HEADERS([config.h])
PKG_PROG_PKG_CONFIG


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

dnl XXX
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


dnl Check for headers and functions
dnl ===============================
AC_CHECK_HEADERS([unistd.h fcntl.h sys/ioctl.h fnmatch.h limits.h wchar.h fts.h execinfo.h])
AC_CHECK_FUNCS([mkdtemp getmntinfo statvfs strtoul lrintf])


### ---------------------------------------------------------------------------
dnl Path settings
dnl =============
AC_PATH_PROG([AUDACIOUS_PATH], [audacious], [no])

AC_ARG_WITH([rc-path], [dnl
  --with-rc-path=path          Resource and configuration path for Audacious
                               relative to $HOME. (deprecated, use XDG)],
    [rc_path=$withval],
    [rc_path=.audacious]
)
AC_DEFINE_UNQUOTED([AUD_RCPATH], "$rc_path",
    [Resource and configuration path for audacious relative to $HOME.]
)
AC_DEFINE([DOTAUDACIOUS_COMPAT], 1, [Enable compatibility for $HOME/.audacious])
AC_SUBST([AUD_RCPATH])


AUD_ARG_ENABLE([one-plugin-dir], [no],
[  --enable-one-plugin-dir      Use a single plugin dir (default=no)])


AUD_ARG_ENABLE([user-plugin-dir], [yes],
[  --disable-user-plugin-dir    Disable per-user plugin dir])

if test "x$enable_user_plugin_dir" = "xno"; then
    AC_DEFINE([DISABLE_USER_PLUGIN_DIR], 1, [Define to disable per user plugin directory])
fi


### ---------------------------------------------------------------------------
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
AUD_CHECK_MODULE([LIBMCS], [libmcs >= 0.7], [libmcs],
    [http://www.atheme.org/projects/mcs.shtml])


### ---------------------------------------------------------------------------
dnl Check if socklen_t is defined
dnl =============================
AUD_TRY_COMPILE([for socklen_t], [have_socklen_t], [
#include <sys/types.h>
#include <sys/socket.h>
], [socklen_t s;],, [
    AC_DEFINE([socklen_t], [int], [Define to int if the socklen_t type is missing])
])


### ---------------------------------------------------------------------------
dnl IPv6 support
dnl ============
AUD_ARG_SIMPLE([ipv6], [no],
[  --enable-ipv6                Enable IPv6 support],
[USE_IPV6],, [Define if building with IPv6 support])



### ---------------------------------------------------------------------------
dnl Plugin stuff
dnl ========================
dnl Ok, first we find out what flags we need to use.
IMPLIB_LIBS=''
AC_SUBST([IMPLIB_LIBS])

dnl XXX
SHARED_SUFFIX=$PLUGIN_SUFFIX
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



### ---------------------------------------------------------------------------
dnl Chardet support
dnl ===============
AUD_ARG_ENABLE([chardet], [yes],
[  --enable-chardet             Enable charset detection support],
[
    AC_DEFINE([USE_CHARDET], 1, [Define if character set detection enabled])
    AC_CHECK_LIB([udet_c], [detectCharset], [
        AC_DEFINE([HAVE_UDET], 1, [Define if the system has Mozilla universal character detector library])
        CHARDET_LIBS="$CHARDET_LIBS -ludet -ludet_c"
        AC_SUBST([CHARDET_LIBS])
    ])
])
AUD_CONDITIONAL([USE_CHARDET], [enable_chardet])


### ---------------------------------------------------------------------------
dnl D-Bus support
dnl =============
AUD_ARG_ENABLE([dbus], [yes],
[  --enable-dbus                Enable D-Bus support],
[
    PKG_CHECK_MODULES([DBUS], [dbus-1 >= 0.60 dbus-glib-1 >= 0.60 gthread-2.0],
        [enable_dbus=yes],
        [enable_dbus=no])
    
    AC_PATH_PROG([DBUS_BINDING_TOOL], [dbus-binding-tool], [no])
    AC_PATH_PROG([GLIB_GENMARSHAL], [glib-genmarshal], [no])
    if test "x$DBUS_BINDING_TOOL" = "xno" || test "x$GLIB_GENMARSHAL" = "xno" || test "x$enable_dbus" = "xno" ; then
        enable_dbus="no"
    else
        AC_DEFINE([USE_DBUS], 1, [Define if D-Bus support enabled])
        AC_DEFINE([DBUS_SERVICES_DIR], "$datadir/dbus-1/services",
            [Location of D-Bus services directory])
        LIBAUDCLIENT_LDFLAGS="-laudclient"
        AC_SUBST([DBUS_CFLAGS])
        AC_SUBST([DBUS_LIBS])
        AC_SUBST([DBUS_SERVICES_DIR])
        AC_SUBST([DBUS_BINDING_TOOL])
        AC_SUBST([LIBAUDCLIENT_LDFLAGS])
    fi
])
AUD_CONDITIONAL([USE_DBUS], [enable_dbus])


### ---------------------------------------------------------------------------
dnl libsamplerate support
dnl =====================
AUD_ARG_ENABLE([samplerate], [no],
[  --enable-samplerate          Enable libsamplerate support],
[
    AUD_CHECK_MODULE([SAMPLERATE], [samplerate],, [libsamplerate])
    aud_save_LIBS="$LIBS"
    AC_CHECK_LIB([samplerate], [src_process],[
    	SAMPLERATE_LIBS="-lsamplerate"
    ], [
        AC_MSG_ERROR([*** Unable to link with libsamplerate!])
    ])
    LIBS="$aud_save_LIBS"
    AC_DEFINE([USE_SAMPLERATE], 1, [Define if libsamplerate enabled])
    AC_SUBST([USE_SAMPLERATE])
    AC_SUBST([SAMPLERATE_CFLAGS])
    AC_SUBST([SAMPLERATE_LIBS])
])


### ---------------------------------------------------------------------------
dnl xspf playlist support
dnl =====================
AUD_ARG_ENABLE([xspf], [yes],
[  --disable-xspf               Disable XSPF support],
[
    AM_PATH_XML2([2.0.0],, [
       AC_MSG_WARN([*** libxml2 is not installed. XSPF playlist support will not be built. ***])
       enable_xspf="no"
    ])
    if test "x$enable_xspf" = "xyes"; then
        AC_DEFINE([HAVE_XSPF_PLAYLIST], 1, [Define if XSPF playlists are available.])
    fi
])


### ---------------------------------------------------------------------------
dnl regex support (gnu/oniguruma/pcre)
dnl ==================================
REGEX_LIBS=
REGEX_CFLAGS=
AC_ARG_WITH([regexlib],
[[  --with-regexlib[=gnu/oniguruma/pcre]   use the chosen regex library (default: gnu)]],
[
case $withval in
  pcre)
    AUD_CHECK_MODULE([LIBPCRE], [libpcre], [>= 6.7], [PCRE])
    AC_DEFINE([USE_REGEX_PCRE], 1, [If this macro is defined, use PCRE regex library.])
    REGEX_LIBS="-lpcreposix $LIBPCRE_LIBS"
    REGEX_CFLAGS=$LIBPCRE_CFLAGS
    ;;
  oniguruma)
    AC_CHECK_LIB([onig], [onig_new],
    [
        AC_DEFINE([USE_REGEX_ONIGURUMA], 1, [If this macro is defined, use Oniguruma regex library.])
        REGEX_LIBS=['-lonig']
    ],[
        AC_MSG_ERROR([Cannot find Oniguruma])
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
dnl SSE2 support
dnl ============
AUD_ARG_ENABLE([sse2], [yes],
[  --disable-sse2               Disable SSE2 support],
[
    AC_MSG_CHECKING([SSE2 support])
    aud_my_save_CFLAGS="$CFLAGS"
    CFLAGS="-msse2"
    AC_TRY_RUN([[
#include <emmintrin.h>
int main()
{
  _mm_setzero_pd();
  return 0;
}
    ]],[
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


### ---------------------------------------------------------------------------
dnl AltiVec support 
dnl ===============
AUD_ARG_ENABLE([altivec], [yes],
[  --disable-altivec            Disable AltiVec support],
[
    AC_CHECK_HEADERS([altivec.h],
    [
        AC_DEFINE([HAVE_ALTIVEC], 1, [Define to 1 if your system has AltiVec.])
        AC_DEFINE([HAVE_ALTIVEC_H], 1, [Define to 1 if your system has an altivec.h file.])
        AC_DEFINE([ARCH_POWERPC], 1, [Define to 1 if your system is a PowerPC.])
        SIMD_CFLAGS=-maltivec
        AC_SUBST([SIMD_CFLAGS])
    ],[
        enable_altivec="no"
    ])
])    


### ---------------------------------------------------------------------------
dnl Session management support
dnl ==========================
AS_AC_EXPAND([desktop_file], "${datadir}/applications/audacious.desktop")
AC_DEFINE_UNQUOTED([AUDACIOUS_DESKTOP_FILE], "${desktop_file}", [Location of the desktop file])

AUD_ARG_ENABLE([sm], [yes],
[  --disable-sm                 Disable session management],
[
    PKG_CHECK_MODULES([SM], [sm >= 1.0], [
        AC_DEFINE([USE_EGGSM], 1, [Define if session management should be used.])
        enable_sm="yes"
    ],[
        enable_sm="no"
    ])
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

AUD_DEFINE_PLUGIN_DIR([Input])
AUD_DEFINE_PLUGIN_DIR([Output])
AUD_DEFINE_PLUGIN_DIR([Effect])
AUD_DEFINE_PLUGIN_DIR([General])
AUD_DEFINE_PLUGIN_DIR([Visualization])
AUD_DEFINE_PLUGIN_DIR([Container])
AUD_DEFINE_PLUGIN_DIR([Transport])
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
    man/audtool.1
    man/audacious.1
    applications/audacious.desktop
    po/Makefile.in
])

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

echo "  Legacy configuration path:              \$HOME/$rc_path"
echo ""
echo "  Use one plugin dir:                     $enable_one_plugin_dir"
echo "  Allow user plugin dir:                  $enable_user_plugin_dir"
echo ""
echo "  Automatic character code detection:     $enable_chardet"
echo "  Sample rate conversion:                 $enable_samplerate"
echo "  D-Bus support:                          $enable_dbus"
echo "  Session management (eggsm)              $enable_sm"
echo "  XSPF playlists                          $enable_xspf"
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
