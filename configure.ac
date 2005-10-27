dnl Process this file with autoconf to produce a configure script.

AC_INIT(audacious, 0.1)
AC_PREREQ(2.5)

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE

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

dnl GNU gettext

AM_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.12.1])


dnl Check for C compiler

AC_LANG([C])
AC_LANG([C++])
AC_ISC_POSIX
AC_C_BIGENDIAN


dnl Check for assembler

AM_PROG_AS


dnl Checks for various programs

AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

dnl Libtool
dnl FIXME: find a way to allow building static libbeep,
dnl without building static plugins

dnl Next four lines is a hack to prevent libtool checking for C++/F77
m4_undefine([AC_PROG_F77])
m4_defun([AC_PROG_F77],[])

AC_DISABLE_STATIC
AC_ENABLE_SHARED
AC_PROG_LIBTOOL

dnl Path settings

AC_PATH_PROG(BEEP_PATH, audacious, no)

AC_ARG_WITH(rc-path,
    [  --rc-path=path                     Resource and configuration path for bmp relative to $HOME.],
    [rc_path=$withval],
    [rc_path=.audacious]
)
AC_DEFINE_UNQUOTED(BMP_RCPATH, "$rc_path",
    [Resource and configuration path for bmp relative to $HOME.]
)
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

AC_ARG_WITH(dev-dsp,
    [  --with-dev-dsp=path                Path to OSS DSP data pipe, default is /dev/dsp.],
    [dev_dsp=$withval],
    [dev_dsp=/dev/dsp]
)
AC_DEFINE_UNQUOTED(DEV_DSP, "$dev_dsp",
    [Path to OSS DSP, really just a data pipe, default is /dev/dsp.]
)

AC_ARG_WITH(dev-mixer,
    [  --with-dev-mixer=path              Path to OSS sound mixer, default is /dev/mixer.],
    [dev_mixer=$withval],
    [dev_mixer=/dev/mixer]
)
AC_DEFINE_UNQUOTED(DEV_MIXER, "$dev_mixer",
    [Path to OSS mixer, default is /dev/mixer.]
)

AC_ARG_WITH(cdda-device,
    [  --with-cdda-device=path            Path to default cdaudio device.],
    [AC_DEFINE_UNQUOTED(CDDA_DEVICE, "$withval",
         [Path to default cdaudio device.]
     )
    ]
)

AC_ARG_WITH(cdda-dir,
    [  --with-cdda-dir=path               Path to default cdaudio directory.],
    [AC_DEFINE_UNQUOTED(CDDA_DIRECTORY, "$withval",
         [Path to default cdaudio directory.]
     )
    ]
)


dnl If {type}_PLUGINS is defined, it should be a comma or space separated
dnl list of plugins that should be built, e.g.,
dnl INPUT_PLUGINS="mpg123,cdaudio cdin".  If variable is not defined, all
dnl buildable plugins are compiled.  This stuff makes life easier for
dnl automated builds that want to disable certain plugins.

for class in EFFECT GENERAL INPUT OUTPUT VISUALIZATION; do
    value=`eval echo "\\$${class}_PLUGINS"`
    if test "${value}" = ""; then
       eval ${class}_PLUGINS="\$\(ALL_PLUGINS\)"
    else
       value=`echo ${value} | sed -e 's/,/ /g'`
       eval ${class}_PLUGINS="\${value}"
    fi
done

AC_SUBST(EFFECT_PLUGINS)
AC_SUBST(GENERAL_PLUGINS)
AC_SUBST(INPUT_PLUGINS)
AC_SUBST(OUTPUT_PLUGINS)
AC_SUBST(VISUALIZATION_PLUGINS)


dnl Option to change equalizer to the old XMMS one which only works with the mpg123
dnl plugin (or rather, only implemented within the plugin)

AC_ARG_WITH(xmms-eq,
    [  --with-xmms-eq                     Use old XMMS equalization code.],
    [use_xmms_eq=$withval],
    [use_xmms_eq=no]
)

if test "$use_xmms_eq" = "yes"; then
   AC_DEFINE(XMMS_EQ, , [Define if building with old XMMS equalization code.])
fi


dnl Check for X
dnl FIXME: remove this dependency

AC_PATH_XTRA
if test "$no_x" = "yes" ; then
    AC_MSG_ERROR([Cannot find X11 headers/libraries])
fi

dnl Check for GTK/GLib/GThread/Pango

PKG_CHECK_MODULES(GTK, [glib-2.0 >= 2.4.0 gtk+-2.0 >= 2.4.0 gthread-2.0 pango],
    [ADD_PC_REQUIRES([glib-2.0 >= 2.4.0, gtk+-2.0 >= 2.4.0])],
    [AC_MSG_ERROR([Cannot find glib2/gtk2/pango])]
)

dnl Check for libglade

PKG_CHECK_MODULES(LIBGLADE, [libglade-2.0 >= 2.3.1],
    [],
    [AC_MSG_ERROR([Cannot find libglade])]
)


dnl x86 SIMD(3DNow!/MMX) instructions support
dnl ========================

AC_ARG_ENABLE(simd,
   [  --enable-simd           build with x86 SIMD(3DNow!/MMX) instructions support (default=no)],
   [USE_SIMD=$enableval],
   [USE_SIMD=no]
)

if test "$USE_SIMD" = "yes"; then
    AC_DEFINE(USE_SIMD, , [Define if building with ia32 SIMD support])
fi

AM_CONDITIONAL(USE_SIMD, test "$USE_SIMD" = "yes")
AC_SUBST(USE_SIMD_TRUE)


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


dnl GConf support

AC_ARG_ENABLE( gconf,
    [  --enable-gconf          enable GConf support (default=disabled)],
    [enable_gconf=$enableval],
    [enable_gconf="no"]
)

if test "$enable_gconf" = "yes"; then
    PKG_CHECK_MODULES(GCONF, [gconf-2.0 >= 2.6.0],
        [
         AC_DEFINE(HAVE_GCONF, , [Define if building with GConf support])
         ADD_PC_REQUIRES([gconf-2.0 >= 2.6.0])
        ],
        [AC_MSG_ERROR([Cannot find GConf])]
    )
fi

AM_CONDITIONAL(HAVE_GCONF, test "$enable_gconf" = "yes")


dnl GnomeVFS support

AC_ARG_ENABLE( gnome-vfs,
    [  --enable-gnome-vfs      enable GnomeVFS support (default=disabled)],
    [enable_gnomevfs=$enableval],
    [enable_gnomevfs="no"]
)

if test "$enable_gnomevfs" = "yes"; then
    PKG_CHECK_MODULES(GNOMEVFS, [gnome-vfs-2.0 >= 2.6.0],
        [
         AC_DEFINE(HAVE_GNOME_VFS, , [Define if building with GnomeVFS support])
         ADD_PC_REQUIRES([gnome-vfs-2.0 >= 2.6.0])
        ],
        [AC_MSG_ERROR([Cannot find GnomeVFS])]
    )
fi

AM_CONDITIONAL(HAVE_GNOME_VFS, test "$enable_gnomevfs" = "yes")


dnl Check for esound

AC_ARG_ENABLE( esd,
    [  --disable-esd           disable esound output plugin [default=enabled]],
    [enable_esd=$enableval],
    [enable_esd="yes"]
)

if test "$enable_esd" = "yes"; then
    PKG_CHECK_MODULES(ESD, [esound >= 0.2],
        [have_esd=yes],
        [have_esd=no]
    )
else
    AC_MSG_RESULT([*** esound plugin disabled per user request ***])
    have_esd=no
fi

AM_CONDITIONAL(HAVE_ESD, test "$have_esd" = "yes")


dnl *** MP3

AC_ARG_ENABLE(mp3,
    [  --disable-mp3           disable mp3 plugin. [default=enabled] ],
    [enable_mp3=$enableval],
    [enable_mp3=yes]
)

AM_CONDITIONAL(ENABLE_MPG123, test "$enable_mp3" = "yes")

dnl *** id3lib
dnl only makes sence if mp3's enabled
if test "x$enable_mp3" = "xyes" ; then

ID3LIBS=""
have_id3lib="no"

dnl here we assume that if libz isn't installed but id3lib is 
dnl it's compiled without compression support
AC_CHECK_HEADERS([id3.h],[
  AC_CHECK_LIB([z],[compress],[ZLIB='-lz'],[ZLIB=''])
  AC_CHECK_LIB([id3],[ID3Tag_New],
      [have_id3lib=yes
       AC_DEFINE(HAVE_ID3LIB,,[Define when using id3lib])
       ID3LIBS="-lstdc++ $ZLIB -lid3"],,[-lstdc++ $ZLIB])])

AC_SUBST(ID3LIBS)

fi dnl $enable_mp3 = yes

dnl *** Ogg Vorbis

AC_ARG_ENABLE(vorbis,
    [  --disable-vorbis        disable Ogg Vorbis input plugin (default=enabled)],
    [enable_vorbis=$enableval],
    [enable_vorbis="yes"]
)

if test "$enable_vorbis" = "yes"; then
    PKG_CHECK_MODULES(OGG_VORBIS, [ogg >= 1.0 vorbis >= 1.0 vorbisfile >= 1.0],
        [have_oggvorbis=yes],
        [AC_MSG_ERROR([Cannot find libogg/libvorbis])
         have_oggvorbis=no]
    )
else
    AC_MSG_RESULT([*** Ogg Vorbis plugin disabled per user request ***])
    have_oggvorbis=no
fi

AM_CONDITIONAL(HAVE_OGGVORBIS, [test "$have_oggvorbis" = "yes"])

dnl *** AAC

AC_ARG_ENABLE(aac,
    [  --disable-aac           disable aac plugin. [default=enabled] ],
    [enable_aac=$enableval],
    [enable_aac=yes]
)

AM_CONDITIONAL(ENABLE_AAC, test "$enable_aac" = "yes")

dnl *** mikmod
AC_ARG_ENABLE( mikmod,
[  --disable-mikmod        Disable mikmod input plugin [default=enabled]],,
                enable_mikmod="yes")

if test "x$enable_mikmod" = xyes; then
        AM_PATH_LIBMIKMOD(3.1.5,have_mikmod=yes, have_mikmod=no)
else
        AC_MSG_RESULT([*** mikmod plugin disabled per user request ***])
        have_mikmod=no
fi
AM_CONDITIONAL(HAVE_MIKMOD,test "x$have_mikmod" = xyes)

dnl *** flac
AC_ARG_ENABLE( flc,
[  --disable-flac        Disable flac input plugin [default=enabled]],,
                enable_flac="yes")

if test "x$enable_flac" = xyes; then
        AM_PATH_LIBFLAC(have_flac=yes, have_flac=no)
else
        AC_MSG_RESULT([*** flac plugin disabled per user request ***])
        have_flac=no
fi
AM_CONDITIONAL(HAVE_FLAC,test "x$have_flac" = xyes)

dnl *** OSS output

AC_ARG_ENABLE(oss,
    [  --disable-oss           disable the OSS output plugin],
    [have_oss=$enableval],
    [have_oss=yes]
)

if test "$have_oss" = "yes"; then
    AC_MSG_CHECKING(for OSS include dir)
    OSS_CFLAGS=""
    if test -f "/etc/oss.conf" ; then
        for i in `cat /etc/oss.conf`; do
            t=`echo $i | sed -e 's/OSSLIBDIR=//'`
            if test "$i" != "$t" ; then
                if test -f "$t/include/sys/soundcard.h" ; then
                    OSS_CFLAGS="-I$t/include"
                fi
            fi
        done
    fi
    if test -n "$OSS_CFLAGS" ; then
        AC_MSG_RESULT([$OSS_CFLAGS])
    else
        AC_MSG_RESULT([not found])
    fi
    CFLAGS_save=$CFLAGS
    CFLAGS="$CFLAGS $OSS_CFLAGS"
    AC_CHECK_HEADERS(sys/soundcard.h)
    AC_CHECK_HEADERS(machine/soundcard.h)
    CFLAGS=$CFLAGS_save

    if test "${ac_cv_header_sys_soundcard_h}" = "yes" || test "${ac_cv_header_machine_soundcard_h}" = "yes"; then
        have_oss=yes
    fi
fi

if test "$have_oss" = "yes"; then
    AC_DEFINE(HAVE_OSS, 1, [Define if the OSS output plugin should be built])
else
    have_oss=no
fi

AM_CONDITIONAL(HAVE_OSS, test "$have_oss" = "yes")


dnl *** ALSA output plugin

AC_ARG_ENABLE(alsa,
    [  --disable-alsa          disable ALSA input plugin (default=enabled)],
    [enable_alsa=$enableval],
    [enable_alsa=yes]
)

if test "$enable_alsa" = "yes"; then
    PKG_CHECK_MODULES(ALSA, [alsa >= 1.0],
        [have_alsa=yes],
        [have_alsa=no]
    )
else
    AC_MSG_RESULT([*** ALSA output plugin disabled per user request ***])
    have_alsa=no
fi

AM_CONDITIONAL(HAVE_ALSA, test "$have_alsa" = "yes")


dnl FreeBSD newpcm driver detection

AC_CACHE_CHECK(for FreeBSD newpcm driver, beep_cv_newpcm_driver,
    if test -r "/dev/sndstat"; then
        grep 'newpcm' /dev/sndstat 2>/dev/null 1>/dev/null
        if test "x$?" = "x0"; then
            beep_cv_newpcm_driver="yes"
        else
            beep_cv_newpcm_driver="no"
        fi
    else
        beep_cv_newpcm_driver="no"
    fi
)

if test "$beep_cv_newpcm_driver" = "yes"; then
    AC_DEFINE(HAVE_NEWPCM, 1, [Define if you have the FreeBSD newpcm driver])
fi


AC_CHECK_HEADERS(linux/cdrom.h)
AC_CHECK_HEADERS(sys/cdio.h)
AC_CHECK_HEADERS(linux/joystick.h)
AC_CHECK_HEADERS(unistd.h)
AC_CHECK_HEADERS(fcntl.h)
AC_CHECK_HEADERS(sys/ioctl.h)
AC_CHECK_HEADERS(fnmatch.h)
AC_CHECK_HEADERS(limits.h)
AC_CHECK_HEADERS(wchar.h)
AC_CHECK_HEADERS(fts.h)

AC_CHECK_FUNCS(mkdtemp)
AC_CHECK_FUNCS(getmntinfo)
AC_CHECK_FUNCS(strtoul)


if test "${ac_cv_header_linux_cdrom_h}" = "yes" || test "${ac_cv_header_sys_cdio_h}" = "yes"
then
    case "$host" in
        *-*-sunos* | *-*-solaris* | *-*-linux*)
            AC_DEFINE(BEEP_CDROM_SOLARIS,, [Define if cdrom access is in Solaris style])
        ;;
        *-*-freebsd*)
            AC_DEFINE(BEEP_CDROM_BSD,, [Define if cdrom access is in BSD style])
        ;;
        *-*-netbsd* | *-*-openbsd*)
            AC_DEFINE(BEEP_CDROM_BSD,, [Define if cdrom access is in BSD style])
            AC_DEFINE(BEEP_CDROM_BSD_NETBSD,, [Define if cdrom access uses NetBSD variant])
        ;;
        *-*darwin*)
            AC_DEFINE(BEEP_CDROM_BSD,, [Define if cdrom access is in BSD style])
            AC_DEFINE(BEEP_CDROM_BSD_DARWIN,, [Define if cdrom access uses Darwin variant])
        ;;
    esac
fi


AM_CONDITIONAL(HAVE_CDROM, test "${ac_cv_header_linux_cdrom_h}" = "yes" || test "${ac_cv_header_sys_cdio_h}" = "yes")
AM_CONDITIONAL(HAVE_LINUX_JOYSTICK,test "${ac_cv_header_linux_joystick_h}" = "yes")

if test -n "$GCC"
then
    CFLAGS="$CFLAGS -Wall -Wpointer-arith -finline-functions -ffast-math"
    case "$CFLAGS" in
        -g*) ;;
        *\ -g*) ;;
        *) CFLAGS="$CFLAGS -fomit-frame-pointer" ;;
    esac
    case "$host" in
        *-*-freebsd*)
        ;;
        *)
        CFLAGS="$CFLAGS -funroll-all-loops"
        ;;
    esac
fi

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
    *-*-openbsd* | *-*-netbsd*)
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
AM_CONDITIONAL(ARCH_X86, test "x$arch_type" = "xix86")
AM_CONDITIONAL(USE_X86ASM, test "x$x86_asm" = xyes)
AM_CONDITIONAL(HAVE_SOLARIS, test "x$have_solaris" = xyes)
AM_CONDITIONAL(HAVE_SUN, test "x$have_sun" = xyes)

plugindir=$libdir/audacious
AC_SUBST(plugindir)

if test "x$enable_one_plugin_dir" = "xyes"; then
    pluginsubs="\\\"Plugins\\\""
    INPUT_PLUGIN_DIR=Plugins
    OUTPUT_PLUGIN_DIR=Plugins
    EFFECT_PLUGIN_DIR=Plugins
    GENERAL_PLUGIN_DIR=Plugins
    VISUALIZATION_PLUGIN_DIR=Plugins
else
    pluginsubs="\\\"Output\\\",\\\"Input\\\",\\\"Effect\\\",\\\"General\\\",\\\"Visualization\\\""
    INPUT_PLUGIN_DIR=Input
    OUTPUT_PLUGIN_DIR=Output
    EFFECT_PLUGIN_DIR=Effect
    GENERAL_PLUGIN_DIR=General
    VISUALIZATION_PLUGIN_DIR=Visualization
fi

AC_SUBST(INPUT_PLUGIN_DIR)
AC_SUBST(OUTPUT_PLUGIN_DIR)
AC_SUBST(EFFECT_PLUGIN_DIR)
AC_SUBST(GENERAL_PLUGIN_DIR)
AC_SUBST(VISUALIZATION_PLUGIN_DIR)
AC_SUBST(pluginsubs)

localedir=$datadir/locale
AC_SUBST(localedir)

BEEP_DEFINES="$CFLAGS -DDATA_DIR=\\\"$datadir/audacious\\\" -DPLUGIN_DIR=\\\"$plugindir\\\" -DPLUGINSUBS=$pluginsubs -DLOCALEDIR=\\\"$localedir\\\""
CFLAGS="$CFLAGS $EXTRA_CFLAGS"

if test "$enable_user_plugin_dir" = "no"; then
    AC_DEFINE(DISABLE_USER_PLUGIN_DIR,, [Define to disable per user plugin directory])
fi

AC_SUBST(BEEP_DEFINES)

PLUGIN_LDFLAGS='-module -avoid-version -export-symbols-regex "get_.plugin_info"'
AC_SUBST(PLUGIN_LDFLAGS)

AC_SUBST(beepdir)
AC_SUBST(plugindir)

have_libvisual="yes"

AC_CHECK_HEADERS([GL/gl.h GL/glx.h GL/glu.h],opengl="yes",opengl="no")
if test x"$opengl" = xno; then
        AC_WARN([OpenGL and Glu headers not found, libvisual support cannot be built])
	have_libvisual="no"
fi

AC_PATH_PROG(SDL_CONFIG, [sdl-config], [no])
if test x$SDL_CONFIG = xno ; then
  AC_WARN([
*** sdl-config not found.
*** If you have installed from binaries, probably you have not
*** installed SDL development package.
*** See http://www.libsdl.org
***
*** Disabling compilation of libvisual-proxy.
])
fi
AM_PATH_SDL(1.2.5, , [AC_WARN([*** SDL >= 1.2.5 not installed - please install first ***])
	have_libvisual="no"])

PKG_CHECK_MODULES(LIBVISUAL, libvisual = 0.2.0, [libvis="yes"], [libvis="no"])
if test x"$libvis" = xno; then
	have_libvisual="no"
fi
AC_SUBST(LIBVISUAL_LIBS)
AC_SUBST(LIBVISUAL_CFLAGS)

AM_CONDITIONAL(HAVE_LIBVISUAL, test "x$have_libvisual" = xyes)

AC_CONFIG_FILES([
        Makefile
        audacious.1
        audacious.spec
        audacious.pc
        audacious/audacious.desktop
        audacious/Makefile
        audacious/glade/Makefile
	audacious/images/Makefile
        libaudacious/Makefile
        Output/Makefile
        Output/OSS/Makefile
        Output/esd/Makefile
        Output/alsa/Makefile
	Output/disk_writer/Makefile
        Input/Makefile
        Input/mpg123/Makefile
	Input/aac/Makefile
	Input/aac/libmp4v2/Makefile
	Input/aac/libfaad2/Makefile
	Input/aac/src/Makefile
	Input/mikmod/Makefile
	Input/tonegen/Makefile
        Input/vorbis/Makefile
        Input/cdaudio/Makefile
        Input/wav/Makefile
	Input/flac/Makefile
	Input/flac/plugin_common/Makefile
        Visualization/Makefile
        Visualization/blur_scope/Makefile
	Visualization/libvisual-proxy/Makefile
        po/Makefile.in
        intl/Makefile
        icons/Makefile
        skin/Makefile
])

AC_OUTPUT


echo
echo "Configuration:"
echo
echo "  Install path:                           ${prefix}"

if test "$BEEP_PATH" != "no"; then
    echo "  Current Audacious executable:           $BEEP_PATH"
fi

echo "  Configuration path:                     \$HOME/$rc_path"
echo
echo "  Use one plugin dir:                     $enable_one_plugin_dir"
echo "  Allow user plugin dir:                  $enable_user_plugin_dir"
echo
echo "  XMMS Legacy"
echo "  -----------"
echo "  Equalization (works only with mpg123)   $use_xmms_eq"
echo
echo "  GNOME support"
echo "  -------------"
echo "  GConf support                           $enable_gconf"
echo "  VFS support                             $enable_gnomevfs"
echo
echo "  Output Plugins"
echo "  --------------"
echo "  Open Sound System (oss):                $have_oss"
echo "  Advanced Linux Sound Arch. (alsa):      $have_alsa"
echo "  Enlightenment Sound Daemon (esd):       $have_esd"
echo
echo "  Input Plugins"
echo "  -------------"
echo "  MPEG 1/2/3 (mpg123):                    $enable_mp3"
echo "    + id3v2 editing:                      $have_id3lib"
echo "  MPEG 4 Audio (AAC):                     $enable_aac"
echo "  Module decoder (mikmod):                $enable_mikmod"
echo "  CD Digital Audio (cdda):                yes"
echo "  Microsoft WAV (wav):                    yes"
echo "  Tone Generator:                         yes"
echo "  Ogg Vorbis (vorbis):                    $have_oggvorbis"
echo "  FLAC:                                   $enable_flac"
echo ""
echo "  Visualization"
echo "  -------------"
echo "  Blur Scope:                             yes"
echo "  libvisual-proxy:                        $have_libvisual"
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
