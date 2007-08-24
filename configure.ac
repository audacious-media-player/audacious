dnl Process this file with autoconf to produce a configure script.
AC_INIT([audacious], [1.4.0], [bugs+audacious@atheme.org])
AC_PREREQ([2.59])

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_CONFIG_HEADERS([config.h])

dnl libbeep
LIBBEEP_MAJOR_VERSION=2
LIBBEEP_MINOR_VERSION=0
LIBBEEP_MICRO_VERSION=0
LIBBEEP_VERSION=$LIBBEEP_MAJOR_VERSION.$LIBBEEP_MINOR_VERSION.$LIBBEEP_MICRO_VERSION
AC_SUBST(LIBBEEP_MAJOR_VERSION)
AC_SUBST(LIBBEEP_MINOR_VERSION)
AC_SUBST(LIBBEEP_MICRO_VERSION)

dnl XXX kludge for FreeBSD
if test -d "/usr/X11R6/include"; then
	CPPFLAGS="$CPPFLAGS -I/usr/X11R6/include"
fi

dnl OMK bootstrap
AC_SUBST([PACKAGE], [AC_PACKAGE_TARNAME])
AC_SUBST([VERSION], [AC_PACKAGE_VERSION])

AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of package])
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version number of package])

dnl GNU gettext

AM_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.12.1])

dnl Check for C compiler

AC_PROG_CC
AC_PROG_CXX
AC_ISC_POSIX
AC_C_BIGENDIAN

test x"$GCC" = x"yes" && CFLAGS="$CFLAGS -Wall -pipe"

dnl Check for assembler

AM_PROG_AS

dnl Checks for various programs

AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AC_PATH_PROG(RM, rm)
AC_PATH_PROG(MV, mv)
AC_PATH_PROG(CP, cp)
AC_PATH_PROG(AR, ar)
AC_PATH_PROG(RANLIB, ranlib)

dnl Path settings

AC_PATH_PROG(BEEP_PATH, audacious, no)

AC_ARG_WITH(rc-path,
    [  --with-rc-path=path                 Resource and configuration path for audacious relative to $HOME. (deprecated, use XDG)],
    [rc_path=$withval],
    [rc_path=.audacious]
)
AC_DEFINE_UNQUOTED(BMP_RCPATH, "$rc_path",
    [Resource and configuration path for audacious relative to $HOME.]
)
AC_DEFINE(DOTAUDACIOUS_COMPAT, 1, [Enable compatibility for $HOME/.audacious])
AC_SUBST(BMP_RCPATH)

AC_ARG_ENABLE(one-plugin-dir,
    [  --enable-one-plugin-dir            Use a single plugin dir [default=no]],
    [enable_one_plugin_dir=$enableval],
    [enable_one_plugin_dir=no]
)

AC_ARG_ENABLE(user-plugin-dir,
    [  --disable-user-plugin-dir          disable per-user plugin dir],
    [enable_user_plugin_dir=$enableval],
    [enable_user_plugin_dir=yes]
)

dnl Check for GTK/GLib/GThread/Pango

PKG_CHECK_MODULES(GTK, [glib-2.0 >= 2.6.0 gtk+-2.0 >= 2.6.0 gthread-2.0 pango],
    [ADD_PC_REQUIRES([glib-2.0 >= 2.6.0, gtk+-2.0 >= 2.6.0])],
    [AC_MSG_ERROR([Cannot find glib2/gtk2/pango])]
)

dnl Check for GLib.

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.6.0],
    [ADD_PC_REQUIRES([glib-2.0 >= 2.6.0])],
    [AC_MSG_ERROR([Cannot find glib2])]
)

dnl Check for libmowgli.

PKG_CHECK_MODULES(MOWGLI, [libmowgli >= 0.4],
    [ADD_PC_REQUIRES([libmowgli >= 0.4])],
    [AC_MSG_ERROR([Cannot find libmowgli: try http://www.atheme-project.org/projects/mowgli.shtml])]
)

dnl Check for libglade

PKG_CHECK_MODULES(LIBGLADE, [libglade-2.0 >= 2.3.1],
    [],
    [AC_MSG_ERROR([Cannot find libglade])]
)

# Check if socklen_t is defined

AC_CACHE_CHECK(for socklen_t, beep_cv_type_socklen_t,
    [AC_TRY_COMPILE(
         [#include <sys/types.h>
          #include <sys/socket.h>],
         [socklen_t s;],
         [beep_cv_type_socklen_t=yes],
         [beep_cv_type_socklen_t=no]
     )
    ]
)

if test "$beep_cv_type_socklen_t" = "no"; then
    AC_DEFINE(socklen_t, int, [Define to int if the socklen_t type is missing])
fi

dnl IPv6 support
dnl ========================
AC_ARG_ENABLE(ipv6,
	[  --enable-ipv6		  enable IPv6 support (default=no)],
			enable_ipv6=$enableval, enable_ipv6=no)
if test "x$enable_ipv6" = xyes; then
    AC_DEFINE(USE_IPV6,, [Define if building with IPv6 support] )
fi
AM_CONDITIONAL(USE_IPV6,test "x$enable_ipv6" = xyes)
AC_SUBST(USE_IPV6)

dnl rpath stuff
dnl ========================
AC_ARG_ENABLE(rpath,
	[  --disable-rpath                disable hardcoded rpath (default=enabled)],
			enable_rpath=$enableval, enable_rpath=yes)

if test "x$enable_rpath" = xyes; then
    RPATH_VAL="-Wl,-rpath=\${libdir}"
fi

dnl plugin stuff
dnl ========================

dnl Ok, first we find out what flags we need to use.
AC_MSG_CHECKING([for what extension and flags to use for plugin compilation])
case "$target" in
	*-apple-*)
		AC_MSG_RESULT([Mac OS X: -fPIC -bundle -fno-common -flat_namespace -undefined suppress, .dylib])
		PICFLAGS="-fPIC -DPIC"
		PICLDFLAGS="-fPIC -DPIC -bundle -fno-common -flat_namespace -undefined suppress"
		LIBLDFLAGS="-dynamiclib -flat_namespace -undefined suppress"
		AUDLDFLAGS=""
		SHARED_SUFFIX=".dylib"
		;;
	*)
		AC_MSG_RESULT([libdl-compatible: -fPIC -DPIC -shared, .so])
		PICFLAGS="-fPIC -DPIC"
		PICLDFLAGS="-fPIC -DPIC -shared"
		LIBLDFLAGS="-fPIC -DPIC -shared"
		AUDLDFLAGS="-Wl,-export-dynamic $RPATH_VAL"
		SHARED_SUFFIX=".so"
		;;
esac
AC_SUBST(PICFLAGS)
AC_SUBST(PICLDFLAGS)
AC_SUBST(LIBLDFLAGS)
AC_SUBST(AUDLDFLAGS)
AC_SUBST(SHARED_SUFFIX)
AC_DEFINE_UNQUOTED(SHARED_SUFFIX, "${SHARED_SUFFIX}",
	[Define the shared module suffix extension on your platform.])

AC_MSG_CHECKING([if you are running Apple-GCC])
case "$target" in
	*-apple-*)
		AC_MSG_RESULT([yes, sorry you poor bastard])
		LDFLAGS="$LDFLAGS -Wl,-framework -Wl,CoreFoundation -Wl,-framework -Wl,CoreServices"
		;;
	*)
		AC_MSG_RESULT([no])
		;;
esac

dnl libguess always compiled in
dnl ========================
SUBDIR_GUESS="libguess"
CHARDET_LIBS="../libguess/libguess.a"

dnl chardet support
dnl ========================
AC_ARG_ENABLE(chardet,
	[  --enable-chardet	  enable character set detection support (default=yes)],
			enable_chardet=$enableval, enable_chardet=yes)
if test "x$enable_chardet" = xyes; then
    AC_DEFINE(USE_CHARDET, 1, [Define if character set detection enabled] )
    AC_CHECK_LIB(udet_c, detectCharset, [AC_DEFINE(HAVE_UDET, 1,[Define if the system has Mozilla universal character detector library]) CHARDET_LIBS=["$CHARDET_LIBS -ludet -ludet_c"]])
fi
AM_CONDITIONAL(USE_CHARDET,test "x$enable_chardet" = xyes)
AC_SUBST(USE_CHARDET)
AC_SUBST(CHARDET_LIBS)
AC_SUBST(SUBDIR_GUESS)

dnl D-Bus support
dnl ========================
AC_ARG_ENABLE(dbus,
    [  --enable-dbus    enable D-Bus support (default=yes)],
    [enable_dbus=$enableval],
    [enable_dbus="yes"]
)

AM_CONDITIONAL(USE_DBUS, test "x$enable_dbus" = "xyes")
if test "x$enable_dbus" = "xyes"; then
	PKG_CHECK_MODULES(DBUS, [dbus-1 >= 0.60 dbus-glib-1 >= 0.60 gthread-2.0],
										enable_dbus="yes", [
		AC_MSG_RESULT(no)
		enable_dbus="no"
	])
	AC_PATH_PROG(DBUS_BINDING_TOOL, dbus-binding-tool, no)
	AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal, no)
	if test "x$DBUS_BINDING_TOOL" = "xno" || test "x$GLIB_GENMARSHAL" = "xno" || test "x$enable_dbus" = "xno" ; then
		enable_dbus="no"
	else
		AC_DEFINE(USE_DBUS, 1, [Define if D-Bus support enabled])
		AC_DEFINE(DBUS_SERVICES_DIR, "$datadir/dbus-1/services",
			[Location of D-Bus services directory])
		USE_DBUS=1
		LIBAUDCLIENT_LDFLAGS="-laudclient"
		AC_SUBST(USE_DBUS)
		AC_SUBST(DBUS_CFLAGS)
		AC_SUBST(DBUS_LIBS)
		AC_SUBST(DBUS_SERVICES_DIR)
		AC_SUBST(DBUS_BINDING_TOOL)
		AC_SUBST(LIBAUDCLIENT_LDFLAGS)
	fi
fi

dnl libsamplerate support
dnl ========================
AC_ARG_ENABLE(samplerate,
  [  --enable-samplerate          enable libsamplerate support (default=no)],
  [enable_samplerate=$enableval], [enable_samplerate=no])
if test "x$enable_samplerate" = "xyes"; then
   AC_DEFINE(USE_SRC, 1, [Define if libsamplerate enabled] )
   PKG_CHECK_MODULES(samplerate, samplerate)
   AC_SUBST(USE_SRC)
   AC_SUBST(samplerate_CFLAGS)
   AC_SUBST(samplerate_LIBS)
   AC_CHECK_LIB(samplerate, src_process,,AC_MSG_ERROR([*** Unable to find libsamplerate library!]))
fi

dnl regex support (gnu/oniguruma/pcre)
dnl ========================
REGEX_LIBS=
REGEX_CFLAGS=
AC_ARG_WITH(regexlib,
[
  --with-regexlib[[=gnu/oniguruma/pcre]]   use the chosen regex library (default: gnu)
],
[case "${withval}" in
  gnu)
    AC_DEFINE(USE_REGEX_GNU,[1],[If this macro is defined, use GNU regex library.])
    ;;
  pcre)
    PKG_CHECK_MODULES(LIBPCRE, [libpcre >= 6.7],
        [
          AC_DEFINE(USE_REGEX_PCRE,[1],[If this macro is defined, use PCRE regex library.])
          REGEX_LIBS="-lpcreposix $LIBPCRE_LIBS"
          REGEX_CFLAGS=$LIBPCRE_CFLAGS
        ],
        [AC_MSG_ERROR([Cannot find PCRE])]
    )
    ;;
  oniguruma)
    AC_CHECK_LIB( onig , onig_new ,
        [
          AC_DEFINE(USE_REGEX_ONIGURUMA,[1],[If this macro is defined, use Oniguruma regex library.])
          REGEX_LIBS=['-lonig']
        ],
        [AC_MSG_ERROR([Cannot find Oniguruma])]
    )
    ;;
  *)
    AC_DEFINE(USE_REGEX_GNU,[1],[If this macro is defined, use GNU regex library.])
    ;;
esac],AC_DEFINE(USE_REGEX_GNU,[1],[If this macro is defined, use GNU regex library.]))

AC_SUBST(REGEX_LIBS)
AC_SUBST(REGEX_CFLAGS)


dnl GConf support

PKG_CHECK_MODULES(LIBMCS, [libmcs >= 0.1],
   [
      AC_DEFINE(HAVE_MCS, , [Define if building with libmcs support])
      ADD_PC_REQUIRES([libmcs >= 0.1])
   ],
   [AC_MSG_ERROR([Cannot find libmcs])]
)

AC_CHECK_HEADERS(unistd.h)
AC_CHECK_HEADERS(fcntl.h)
AC_CHECK_HEADERS(sys/ioctl.h)
AC_CHECK_HEADERS(fnmatch.h)
AC_CHECK_HEADERS(limits.h)
AC_CHECK_HEADERS(wchar.h)
AC_CHECK_HEADERS(fts.h)
AC_CHECK_HEADERS(execinfo.h)

AC_CHECK_FUNCS([mkdtemp getmntinfo statvfs strtoul lrintf])

AC_MSG_CHECKING(if __ELF__ is defined)
AC_EGREP_CPP(yes,
    [#ifdef __ELF__
        yes
     #endif
    ],
    [is_elf=yes],
    [is_elf=no]
)
AC_MSG_RESULT([${is_elf}])

have_solaris=no
have_sun=no
ARCH_DEFINES=""
case "$host" in
    *-*-openbsd* | *-*-netbsd* | *-*-mirbsd*)
        have_sun=yes
        if test "${is_elf}" = "no"
        then
            AC_DEFINE(SYMBOL_PREFIX, "_", [Define to symbol prefix, if any])
        fi
    ;;
    *-*-darwin*)
        AC_DEFINE(SYMBOL_PREFIX, "_", [Define to symbol prefix, if any])
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
case "$host" in
    i386-*-* | i486-*-* | i586-*-* | i686-*-* | i86pc-*-*)
        arch_type=ix86
        case "$host" in
            *-*-netbsd* | *-*openbsd*)
            ;;
            *)
                ARCH_DEFINES="-DI386_ASSEM"
                x86_asm=yes
            ;;
        esac
    ;;
esac

AC_SUBST(ARCH_DEFINES)

dnl XXX delete ASAP -nenolod
AM_CONDITIONAL(ARCH_X86, test "x$arch_type" = "xix86")
AM_CONDITIONAL(USE_X86ASM, test "x$x86_asm" = xyes)
AM_CONDITIONAL(HAVE_SOLARIS, test "x$have_solaris" = xyes)
AM_CONDITIONAL(HAVE_SUN, test "x$have_sun" = xyes)

plugindir="$libdir/audacious"
AC_SUBST(plugindir)

if test "x$enable_one_plugin_dir" = "xyes"; then
    pluginsubs="\\\"Plugins\\\""
    INPUT_PLUGIN_DIR=Plugins
    OUTPUT_PLUGIN_DIR=Plugins
    EFFECT_PLUGIN_DIR=Plugins
    GENERAL_PLUGIN_DIR=Plugins
    VISUALIZATION_PLUGIN_DIR=Plugins
    CONTAINER_PLUGIN_DIR=Plugins
    TRANSPORT_PLUGIN_DIR=Plugins
    DISCOVERY_PLUGIN_DIR=Plugins
else
    pluginsubs="\\\"Output\\\",\\\"Input\\\",\\\"Effect\\\",\\\"General\\\",\\\"Visualization\\\",\\\"Container\\\",\\\"Transport\\\",\\\"Discovery\\\""
    INPUT_PLUGIN_DIR=Input
    OUTPUT_PLUGIN_DIR=Output
    EFFECT_PLUGIN_DIR=Effect
    GENERAL_PLUGIN_DIR=General
    VISUALIZATION_PLUGIN_DIR=Visualization
    CONTAINER_PLUGIN_DIR=Container
    TRANSPORT_PLUGIN_DIR=Transport
    DISCOVERY_PLUGIN_DIR=Discovery
fi

AC_SUBST(INPUT_PLUGIN_DIR)
AC_SUBST(OUTPUT_PLUGIN_DIR)
AC_SUBST(EFFECT_PLUGIN_DIR)
AC_SUBST(GENERAL_PLUGIN_DIR)
AC_SUBST(VISUALIZATION_PLUGIN_DIR)
AC_SUBST(CONTAINER_PLUGIN_DIR)
AC_SUBST(TRANSPORT_PLUGIN_DIR)
AC_SUBST(pluginsubs)

dnl XXX Work around some autoconf bugs.
if test "$prefix" = "NONE"; then
        prefix="${ac_default_prefix}"
fi

if test -z "$datadir"; then
	datadir="${prefix}/share"
fi

localedir="$datadir/locale"
AC_SUBST(localedir)

BEEP_DEFINES="$CFLAGS -DDATA_DIR=\\\"$datadir/audacious\\\" -DPLUGIN_DIR=\\\"$plugindir\\\" -DPLUGINSUBS=$pluginsubs -DLOCALEDIR=\\\"$localedir\\\""
CFLAGS="$CFLAGS $EXTRA_CFLAGS"

if test "$enable_user_plugin_dir" = "no"; then
    AC_DEFINE(DISABLE_USER_PLUGIN_DIR,, [Define to disable per user plugin directory])
fi

AC_SUBST(BEEP_DEFINES)

AC_ARG_ENABLE(xspf,
[  --disable-xspf          disable XSPF support. (default=enabled)],
[have_xspf=$enableval], [have_xspf=yes])

AM_PATH_XML2(2.0.0, , [
       AC_MSG_WARN([*** libxml2 is not installed. Paranormal nor XSPF support will not be built. ***])
       have_xspf="no"
])

if test "$have_xspf" = "yes"; then
       AC_DEFINE(HAVE_XSPF_PLAYLIST, 1, [Define if XSPF playlists are available.])
fi

AC_SUBST(beepdir)
AC_SUBST(plugindir)

AC_CONFIG_FILES([
	audacious.pc
	audclient.pc
	mk/rules.mk
	man/audtool.1
	man/audacious.1
	src/audacious/audacious.desktop
	po/Makefile.in
])

AC_OUTPUT


echo
echo "Configuration:"
echo
echo "  Install path:                           ${prefix}"

if test "$BEEP_PATH" != "no"; then
    echo "  Current Audacious executable:           $BEEP_PATH"
fi

echo "  Legacy configuration path:              \$HOME/$rc_path"
echo
echo "  Use one plugin dir:                     $enable_one_plugin_dir"
echo "  Allow user plugin dir:                  $enable_user_plugin_dir"
echo 
echo "  Automatic character code detection:     $enable_chardet"
echo "  Sample rate upconversion:               $enable_samplerate"
echo "  D-Bus support:                          $enable_dbus"
echo

if test "$beep_cv_lib_xlibs_threadsafe" = "no"; then
    echo "Note: You need to have thread-safe xlibs to use Audacious."
    echo "If you are using libc5 thread aware Xlibs just skip this."
    echo ""
    echo "If you don't have those libs and you are on a libc5 system get them at:"
    echo "ftp://ftp.dent.med.uni-muenchen.de/pub/wmglo/"
fi

if test "$BEEP_PATH" != "no" && test "$prefix/bin/audacious" != "$BEEP_PATH" ; then
    echo ""
    echo "Note: Configure has discovered that you already have Audacious installed"
    echo "and it does not match with the given --prefix. You have Audacious installed "
    echo "in $(dirname $BEEP_PATH) and you chose $prefix/bin."
    echo
    echo "If you don't want two copies of Audacious installed, rerun configure with"
    echo "the --prefix option set to the location of the old Audacious, or uninstall"
    echo "the old Audacious."
fi
