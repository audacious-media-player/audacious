
dnl Process this file with autoconf to produce a configure script.
AC_INIT([audacious], [1.9.0], [bugs+audacious@atheme.org])
AC_PREREQ([2.59])

AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AC_GNU_SOURCE
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

test "x$GCC" = "xyes" && CFLAGS="$CFLAGS -Wall -pipe"

dnl Check for assembler

AM_PROG_AS

dnl Checks for various programs

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

PKG_CHECK_MODULES(GLIB, [gthread-2.0],
    [ADD_PC_REQUIRES([gthread-2.0 >= 2.14.0])],
    [AC_MSG_ERROR([Cannot find gthread-2.0 >= 2.14.0])]
)

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.14.0],
    [ADD_PC_REQUIRES([glib-2.0 >= 2.14.0])],
    [AC_MSG_ERROR([Cannot find Glib2 >= 2.14.0])]
)

PKG_CHECK_MODULES(GTK, [gtk+-2.0 >= 2.10.0],
    [ADD_PC_REQUIRES([gtk+-2.0 >= 2.10.0])],
    [AC_MSG_ERROR([Cannot find Gtk+2 >= 2.10.0])]
)

PKG_CHECK_MODULES(PANGO, [pango >= 1.8.0],
    [ADD_PC_REQUIRES([pango >= 1.8.0])],
    [AC_MSG_ERROR([Cannot find Pango >= 1.8.0])]
)


dnl Check for libmowgli.

PKG_CHECK_MODULES(MOWGLI, [libmowgli >= 0.4.0],
    [ADD_PC_REQUIRES([libmowgli >= 0.4.0])],
    [AC_MSG_ERROR([Cannot find libmowgli, try http://www.atheme.org/projects/mowgli.shtml])]
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
if test "x$enable_ipv6" = "xyes"; then
    AC_DEFINE(USE_IPV6,, [Define if building with IPv6 support] )
fi
AM_CONDITIONAL(USE_IPV6,test "x$enable_ipv6" = "xyes")
AC_SUBST(USE_IPV6)

dnl PCH support
dnl ========================
AC_ARG_ENABLE(pch,
	[  --enable-pch                   use GCC precompiled headers (experimental) (default=no)],
			enable_pch=$enableval, enable_pch=no)
if test "x$enable_pch" = "xyes"; then
   PCH="pch"
fi
AC_SUBST(PCH)

dnl plugin stuff
dnl ========================

dnl Ok, first we find out what flags we need to use.
BUILDSYS_PROG_IMPLIB
BUILDSYS_SHARED_LIB
IMPLIB_LIBS=''
AC_SUBST(IMPLIB_LIBS)

dnl XXX
SHARED_SUFFIX=$PLUGIN_SUFFIX
AC_SUBST(SHARED_SUFFIX)
AC_DEFINE_UNQUOTED(SHARED_SUFFIX, "${SHARED_SUFFIX}",
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

dnl chardet support
dnl ========================
AC_ARG_ENABLE(chardet,
	[  --enable-chardet	  enable character set detection support (default=yes)],
			enable_chardet=$enableval, enable_chardet=yes)
if test "x$enable_chardet" = "xyes"; then
    AC_DEFINE(USE_CHARDET, 1, [Define if character set detection enabled] )
    AC_CHECK_LIB(udet_c, detectCharset, [AC_DEFINE(HAVE_UDET, 1,[Define if the system has Mozilla universal character detector library]) CHARDET_LIBS=["$CHARDET_LIBS -ludet -ludet_c"]])
fi
AM_CONDITIONAL(USE_CHARDET,test "x$enable_chardet" = "xyes")
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


dnl mcs support

PKG_CHECK_MODULES(LIBMCS, [libmcs >= 0.7],
   [
      AC_DEFINE(HAVE_MCS, , [Define if building with libmcs support])
      ADD_PC_REQUIRES([libmcs >= 0.7])
   ],
   [AC_MSG_ERROR([Cannot find libmcs, try http://www.atheme.org/projects/mcs.shtml])]
)

dnl AltiVec support 

AC_ARG_ENABLE(altivec,
    [  --disable-altivec       disable AltiVec support. (default=enabled) ],
    [enable_altivec=$enableval],
    [enable_altivec=yes]
)
if test "x$enable_altivec" = "xyes"; then
  AC_CHECK_HEADERS(altivec.h,
      [AC_DEFINE(HAVE_ALTIVEC, 1, [Define to 1 if your system has AltiVec.])
       AC_DEFINE(HAVE_ALTIVEC_H, 1, [Define to 1 if your system has an altivec.h file.])
       AC_DEFINE(ARCH_POWERPC, 1, [Define to 1 if your system is a PowerPC.])
       SIMD_CFLAGS=-maltivec
       AC_SUBST(SIMD_CFLAGS)],
      [enable_altivec="no"]
  )    
fi

dnl SSE2 support

AC_ARG_ENABLE(sse2,
    [  --disable-sse2          disable SSE2 support. (default=enabled) ],
    [enable_sse2=$enableval],
    [enable_sse2=yes]
)
if test "x$enable_sse2" = "xyes"; then
  AC_MSG_CHECKING(SSE2)
  ac_save_CFLAGS="$CFLAGS"
  CFLAGS="-msse2"
  AC_TRY_RUN([
  #include <emmintrin.h>
  int main()
  {
    _mm_setzero_pd();
    return 0;
  }
  ],
  [AC_MSG_RESULT(SSE2 yes)
  AC_DEFINE(HAVE_SSE2, 1, [Define to 1 if your system has SSE2])
  SIMD_CFLAGS=-msse2],
  [AC_MSG_RESULT(SSE2 no)
  enable_sse2="no"],
  )
fi
AC_SUBST(SIMD_CFLAGS)
CFLAGS="$ac_save_CFLAGS"


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
AM_CONDITIONAL(USE_X86ASM, test "x$x86_asm" = "xyes")
AM_CONDITIONAL(HAVE_SOLARIS, test "x$have_solaris" = "xyes")
AM_CONDITIONAL(HAVE_SUN, test "x$have_sun" = "xyes")

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

dnl setup rpath if prefix or exec-prefix is specified
if test "x$prefix" != "xNONE" || test "x$exec_prefix" != "xNONE" ; then
	test "x`$CC -dM -E - </dev/null | grep __ELF__`" != "x" && RPATH="-Wl,-rpath,$libdir"
fi

dnl XXX Work around some autoconf bugs.
if test "x$prefix" = "xNONE"; then
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

if test "x$have_xspf" = "xyes"; then
       AC_DEFINE(HAVE_XSPF_PLAYLIST, 1, [Define if XSPF playlists are available.])
fi

dnl disable rpath
AC_ARG_ENABLE(rpath,
	      [  --disable-rpath	  force to disable -rpath. (-rpath is required and automatically appended when --prefix or --exec-prefix is specified on ELF systems.)], [enable_rpath=$enableval], [enable_rpath=yes])
if test "x$enable_rpath" = "xyes"; then
	LDFLAGS="$LDFLAGS $RPATH"
fi

dnl AUDACIOUS_DESKTOP_FILE check
dnl ========================
AS_AC_EXPAND(desktop_file,"${datadir}/applications/audacious.desktop")
AC_DEFINE_UNQUOTED(AUDACIOUS_DESKTOP_FILE,"${desktop_file}",["The location of the desktop file"])

dnl libSM check
PKG_CHECK_MODULES(sm, sm)

AC_SUBST(beepdir)
AC_SUBST(plugindir)

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


echo
echo "Configuration:"
echo
echo "  Install path:                           ${prefix}"

if test "x$BEEP_PATH" != "xno"; then
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
echo "  SSE2:                                   $enable_sse2"
echo "  AltiVec:                                $enable_altivec"
echo

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
