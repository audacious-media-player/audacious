dnl Process this file with autoconf to produce a configure script.

AC_INIT(audacious, 1.1.1)
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

AM_INIT_AUTOMAKE

dnl GNU gettext

AM_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.12.1])


dnl Check for C compiler

AC_PROG_CC
AC_PROG_CXX
AC_ISC_POSIX
AC_C_BIGENDIAN

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
    [  --rc-path=path                 Resource and configuration path for audacious relative to $HOME.],
    [rc_path=$withval],
    [rc_path=.audacious]
)
AC_DEFINE_UNQUOTED(BMP_RCPATH, "$rc_path",
    [Resource and configuration path for audacious relative to $HOME.]
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
    [  --with-dev-dsp=path            Path to OSS DSP data pipe, default is /dev/dsp.],
    [dev_dsp=$withval],
    [dev_dsp=/dev/dsp]
)
AC_DEFINE_UNQUOTED(DEV_DSP, "$dev_dsp",
    [Path to OSS DSP, really just a data pipe, default is /dev/dsp.]
)

AC_ARG_WITH(dev-mixer,
    [  --with-dev-mixer=path          Path to OSS sound mixer, default is /dev/mixer.],
    [dev_mixer=$withval],
    [dev_mixer=/dev/mixer]
)
AC_DEFINE_UNQUOTED(DEV_MIXER, "$dev_mixer",
    [Path to OSS mixer, default is /dev/mixer.]
)

AC_ARG_WITH(cdda-device,
    [  --with-cdda-device=path        Path to default cdaudio device.],
    [AC_DEFINE_UNQUOTED(CDDA_DEVICE, "$withval",
         [Path to default cdaudio device.]
     )
    ]
)

AC_ARG_WITH(cdda-dir,
    [  --with-cdda-dir=path           Path to default cdaudio directory.],
    [AC_DEFINE_UNQUOTED(CDDA_DIRECTORY, "$withval",
         [Path to default cdaudio directory.]
     )
    ]
)

dnl These plugins are always built.

INPUT_PLUGINS="cdaudio tonegen console sexypsf wav cue"
OUTPUT_PLUGINS="disk_writer"
EFFECT_PLUGINS="audiocompress ladspa voice_removal"
GENERAL_PLUGINS="song_change"
VISUALIZATION_PLUGINS="blur_scope"
CONTAINER_PLUGINS="m3u pls"

dnl Option to change equalizer to the old XMMS one which only works with the mpg123
dnl plugin (or rather, only implemented within the plugin)

AC_ARG_WITH(xmms-eq,
    [  --with-xmms-eq                 Use old XMMS equalization code.],
    [use_xmms_eq=$withval],
    [use_xmms_eq=no]
)

if test "$use_xmms_eq" = "yes"; then
   AC_DEFINE(XMMS_EQ, , [Define if building with old XMMS equalization code.])
fi

dnl Check for GTK/GLib/GThread/Pango

PKG_CHECK_MODULES(GTK, [glib-2.0 >= 2.6.0 gtk+-2.0 >= 2.6.0 gthread-2.0 pango],
    [ADD_PC_REQUIRES([glib-2.0 >= 2.6.0, gtk+-2.0 >= 2.6.0])],
    [AC_MSG_ERROR([Cannot find glib2/gtk2/pango])]
)

dnl Check for libglade

PKG_CHECK_MODULES(LIBGLADE, [libglade-2.0 >= 2.3.1],
    [],
    [AC_MSG_ERROR([Cannot find libglade])]
)

AC_PATH_PROG(TAGLIB_CONFIG, taglib-config, [no], [$PATH:$prefix/bin])
if test "x$TAGLIB_CONFIG" = "xno" ; then
AC_MSG_ERROR([Cannot find taglib])
else
    TAGLIB_CFLAGS=[`$TAGLIB_CONFIG --cflags`]
    TAGLIB_LIBS=[`$TAGLIB_CONFIG --libs`]
fi
AC_SUBST(TAGLIB_CFLAGS)
AC_SUBST(TAGLIB_LIBS)

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

dnl chardet support
dnl ========================
AC_ARG_ENABLE(chardet,
	[  --enable-chardet	 enable character set detection support (default=no)],
			enable_chardet=$enableval, enable_chardet=no)
if test "x$enable_chardet" = xyes; then
    AC_DEFINE(USE_CHARDET, 1, [Define if character set detection enabled] )
    AC_CHECK_LIB(guess, guess_jp, [SUBDIR_GUESS=[''] CHARDET_LIBS=['-lguess']], [SUBDIR_GUESS=['libguess'] CHARDET_LIBS=['../libguess/libguess.a']])
    AC_CHECK_LIB(udet_c, detectCharset, [AC_DEFINE(HAVE_UDET, 1,[Define if the system has Mozilla universal character detector library]) CHARDET_LIBS=["$CHARDET_LIBS -ludet -ludet_c"]])
fi
AM_CONDITIONAL(USE_CHARDET,test "x$enable_chardet" = xyes)
AC_SUBST(USE_CHARDET)
AC_SUBST(CHARDET_LIBS)
AC_SUBST(SUBDIR_GUESS)

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

if test "$enable_gconf" = "yes"; then
	CONFIGDB_BACKEND="gconf"
else
	CONFIGDB_BACKEND="rcfile"
fi

AC_SUBST(CONFIGDB_BACKEND)

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

if test "$enable_gnomevfs" = "yes"; then
	VFS_BACKEND="gnome"
else
	VFS_BACKEND="stdio"
fi

AC_SUBST(VFS_BACKEND)

dnl Check for esound

AC_ARG_ENABLE( esd,
    [  --disable-esd           disable esound output plugin [default=enabled]],
    [enable_esd=$enableval],
    [enable_esd="yes"]
)

if test "$enable_esd" = "yes"; then
    PKG_CHECK_MODULES(ESD, [esound >= 0.2],
        [have_esd=yes
         OUTPUT_PLUGINS="$OUTPUT_PLUGINS esd"],
        [have_esd=no]
    )
else
    AC_MSG_RESULT([*** esound plugin disabled per user request ***])
    have_esd=no
fi

dnl *** AltiVec

AC_CHECK_HEADERS(altivec.h,
    [AC_DEFINE(HAVE_ALTIVEC, 1, [Define to 1 if your system has AltiVec.])
     AC_DEFINE(HAVE_ALTIVEC_H, 1, [Define to 1 if your system has an altivec.h file.])
     AC_DEFINE(ARCH_POWERPC, 1, [Define to 1 if your system is a PowerPC.])
     DCT64=dct64_altivec.c],
    [DCT64=dct64.c]
)    
AC_SUBST(DCT64)

dnl *** MP3

AC_ARG_ENABLE(mp3,
    [  --disable-mp3           disable mp3 plugin. (default=enabled) ],
    [enable_mp3=$enableval],
    [enable_mp3=yes]
)

if test "$enable_mp3" = "yes"; then
	INPUT_PLUGINS="$INPUT_PLUGINS mpg123"
fi

dnl *** ThinkLight support

AC_CHECK_FILE([/proc/acpi/ibm/light], [VISUALIZATION_PLUGINS="$VISUALIZATION_PLUGINS rocklight"])

dnl *** LIRC client libraries

AC_ARG_ENABLE(lirc,
    [  --disable-lirc          disable LIRC plugin (default=enabled)],
    [enable_lirc=$enableval],
    [enable_lirc="yes"]
)

if test "$enable_lirc" = "yes"; then
    AC_CHECK_LIB([lirc_client],[lirc_init],[have_lirc=yes],[have_lirc=no])
else
    AC_MSG_RESULT([*** LIRC plugin disabled per user request ***])
    have_lirc="no"
fi

if test "$have_lirc" = "yes"; then
	GENERAL_PLUGINS="$GENERAL_PLUGINS lirc"
fi

dnl *** libnotify requirement

AC_ARG_ENABLE(notify,
    [  --disable-notify        disable libnotify plugin (default=enabled)],
    [enable_libnotify=$enableval],
    [enable_libnotify="yes"]
)

if test "$enable_libnotify" = "yes"; then
    PKG_CHECK_MODULES(LIBNOTIFY, [libnotify >= 0.4.2],
        [have_libnotify=yes
         GENERAL_PLUGINS="$GENERAL_PLUGINS notify"],
        [have_libnotify=no]
    )
else
    AC_MSG_RESULT([*** libnotify plugin disabled per user request ***])
    have_libnotify="no"
fi

dnl *** AdPlug requirement (libbinio)

AC_ARG_ENABLE(adplug,
    [  --disable-adplug        disable AdPlug plugin (default=enabled)],
    [enable_adplug=$enableval],
    [enable_adplug="yes"]
)

if test "$enable_adplug" = "yes"; then
    have_adplug="yes"
    PKG_CHECK_MODULES(BINIO,[libbinio >= 1.4],,[have_adplug=no])
else
    AC_MSG_RESULT([*** AdPlug plugin disabled per user request ***])
    have_adplug="no"
fi

if test "$have_adplug" = "yes"; then
	INPUT_PLUGINS="$INPUT_PLUGINS adplug"
fi

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

if test "$have_oggvorbis" = "yes"; then
	INPUT_PLUGINS="$INPUT_PLUGINS vorbis"
fi

dnl *** AAC

AC_ARG_ENABLE(aac,
    [  --disable-aac           disable aac plugin (default=enabled) ],
    [enable_aac=$enableval],
    [enable_aac=yes]
)

if test "$enable_aac" = "yes"; then
	INPUT_PLUGINS="$INPUT_PLUGINS aac"
fi

dnl *** sndfile

AC_ARG_ENABLE(sndfile,
    [  --disable-sndfile       disable sndfile extensions. [default=enabled] ],
    [enable_sndfile=$enableval],
    [enable_sndfile=yes]
)

if test "$enable_sndfile" = "yes"; then
    PKG_CHECK_MODULES(SNDFILE, [sndfile >= 0.19],
        [enable_sndfile=yes],
        [enable_sndfile=no]
    )
else
    AC_MSG_RESULT([*** libsndfile extensions disabled per user request ***])
    enable_sndfile=no
fi

if test "$enable_sndfile" = "yes"; then
	WAV_SNDFILE="-sndfile"
fi

AC_SUBST(WAV_SNDFILE)

dnl *** modplug

AC_ARG_ENABLE(modplug,
    [  --disable-modplug       disable ModPlug plugin (default=enabled)],
    [enable_modplug=$enableval],
    [enable_modplug="yes"]
)

if test "$enable_modplug" = "yes"; then
    AC_CHECK_LIB([modplug],[XMLinearTable],[have_modplug=yes],[have_modplug=no],[-lstdc++])
else
    AC_MSG_RESULT([*** ModPlug plugin disabled per user request ***])
    have_modplug="no"
fi

if test "$have_modplug" = "yes"; then
	INPUT_PLUGINS="$INPUT_PLUGINS modplug"
fi

dnl *** flac
AC_ARG_ENABLE( flac,
[  --disable-flac          disable flac input plugin (default=enabled)],,
                enable_flac="yes")

if test "x$enable_flac" = xyes; then
        AM_PATH_LIBFLAC(have_flac=yes, have_flac=no)
else
        AC_MSG_RESULT([*** flac plugin disabled per user request ***])
        have_flac=no
fi

if test "$have_flac" = "yes"; then
	INPUT_PLUGINS="$INPUT_PLUGINS flac"
fi

dnl *** WMA

AC_ARG_ENABLE(wma,
    [  --disable-wma           disable wma plugin. (default=enabled)],
    [enable_wma=$enableval],
    [enable_wma=yes]
)

if test "$enable_wma" = "yes"; then
	INPUT_PLUGINS="$INPUT_PLUGINS wma"
fi

dnl *** jack output plugin
AC_ARG_ENABLE( jack,
[  --disable-jack          disable jack output plugin (default=enabled)],,
                enable_jack="yes")

if test "x$enable_jack" = xyes; then
        AM_PATH_JACK(have_jack=yes, have_jack=no)
else
        AC_MSG_RESULT([*** jack plugin disabled per user request ***])
        have_jack=no
fi

if test "$have_jack" = yes; then
	OUTPUT_PLUGINS="$OUTPUT_PLUGINS jack"
fi

dnl *** arts output plugin
AC_ARG_ENABLE( arts,
[  --disable-arts          disable arts output plugin (default=enabled)],,
                enable_arts="yes")

if test "x$enable_arts" = xyes; then
        AM_PATH_ARTSC(0.9.5, have_arts=yes, have_arts=no)
else
        AC_MSG_RESULT([*** arts plugin disabled per user request ***])
        have_arts=no
fi

if test "$have_arts" = yes; then
	OUTPUT_PLUGINS="$OUTPUT_PLUGINS arts"
fi

dnl *** sid
AC_ARG_ENABLE( sid,
[  --disable-sid           disable sid input plugin (default=enabled)],,
                enable_sid="yes")

if test "x$enable_sid" = xyes; then
        AM_PATH_SIDPLAY(have_sidplay=yes, have_sidplay=no)
else
        AC_MSG_RESULT([*** sid plugin disabled per user request ***])
        have_sidplay=no
fi

if test "$have_sidplay" = yes; then
	INPUT_PLUGINS="$INPUT_PLUGINS sid"
fi

dnl *** Musepack

AC_ARG_ENABLE(musepack,
    [  --disable-musepack      disable musepack input plugin (default=enabled)],
    [enable_musepack=$enableval],
    [enable_musepack="yes"]
)

if test "$enable_musepack" = "yes"; then
    AC_CHECK_HEADERS(mpcdec/config_types.h,,
        enable_musepack=no
    )
else
    AC_MSG_RESULT([*** Musepack plugin disabled per user request ***])
    enable_musepack=no
fi

if test "$enable_musepack" = "yes"; then
	INPUT_PLUGINS="$INPUT_PLUGINS musepack"
fi

dnl *** OSS output

AC_ARG_ENABLE(oss,
    [  --disable-oss           disable the OSS output plugin (default=enabled)],
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

if test "$have_oss" = "yes"; then
	OUTPUT_PLUGINS="$OUTPUT_PLUGINS OSS"
fi

dnl *** ALSA output plugin

AC_ARG_ENABLE(alsa,
    [  --disable-alsa          disable ALSA input plugin (default=enabled)],
    [enable_alsa=$enableval],
    [enable_alsa=yes]
)

if test "$enable_alsa" = "yes"; then
    PKG_CHECK_MODULES(ALSA, [alsa >= 1.0],
[
have_alsa=yes
alsalib_available=yes
],
[
have_alsa=no
alsalib_available=no
]
    )
else
    AC_MSG_RESULT([*** ALSA output plugin disabled per user request ***])
    have_alsa=no
    alsalib_available=check
fi

if test "$have_alsa" = "yes"; then
	OUTPUT_PLUGINS="$OUTPUT_PLUGINS alsa"
fi


dnl *** amidi-plug (note: to avoid checking twice ALSA, this should appear somewhere after the alsa ouput plugin check)

AC_ARG_ENABLE(amidiplug,
    [  --disable-amidiplug     disable amidi-plug input plugin (default=check) ],
    [enable_amidiplug=$enableval],
    [enable_amidiplug=check]
)

AC_ARG_ENABLE(amidiplug-alsa,
    [  --disable-amidiplug-alsa    disable amidi-plug ALSA backend (default=enabled) ],
    [enable_amidiplug_alsa=$enableval],
    [enable_amidiplug_alsa=auto]
)

AC_ARG_ENABLE(amidiplug-flsyn,
    [  --disable-amidiplug-flsyn   disable amidi-plug FluidSynth backend (default=enabled) ],
    [enable_amidiplug_fluidsynth=$enableval],
    [enable_amidiplug_fluidsynth=auto]
)

AC_ARG_ENABLE(amidiplug-dummy,
    [  --disable-amidiplug-dummy   disable amidi-plug dummy backend (default=enabled) ],
    [enable_amidiplug_dummy=$enableval],
    [enable_amidiplug_dummy=auto]
)


if test "x$enable_amidiplug" = "xno"; then
  AC_MSG_RESULT([*** amidi-plug disabled per user request ***])
  enable_amidiplug=no
else
  if test "x$enable_amidiplug_alsa" != "xauto"; then
    enable_amidiplug=yes
  elif test "x$enable_amidiplug_fluidsynth" != "xauto"; then
    enable_amidiplug=yes
  elif test "x$enable_amidiplug_dummy" != "xauto"; then
    enable_amidiplug=yes
  fi
fi

if test "x$enable_amidiplug" = "xcheck"; then
  if test "x$alsalib_available" = "xcheck"; then
    PKG_CHECK_MODULES(ALSA, [alsa >= 1.0],
      [alsalib_available=yes],[alsalib_available=no])
  fi
  if test "x$alsalib_available" = "xyes"; then
    AM_PATH_AMIDIPLUG_HWSYNTH(ap_have_hwsynth=yes, ap_have_hwsynth=no)
    if test "x$ap_have_hwsynth" = "xyes"; then
      enable_amidiplug=yes
      ap_disable_timidity=yes
    else
      PKG_CHECK_MODULES(FLUIDSYNTH, [fluidsynth >= 1.0.6],
        [fluidsynth_available=yes],[fluidsynth_available=no])
      if test "x$fluidsynth_available" = "xyes"; then
        enable_amidiplug=yes
        ap_disable_timidity=yes
      else
        enable_amidiplug=no
        AC_MSG_WARN([*** Neither an ALSA-supported hardware synth or FluidSynth software synth (>= 1.0.6) have been found (even if ALSA was detected), so amidi-plug won't be compiled unless you explicitly request it with --enable-amidiplug ***])
      fi
    fi
  else
    PKG_CHECK_MODULES(FLUIDSYNTH, [fluidsynth >= 1.0.6],
      [fluidsynth_available=yes],[fluidsynth_available=no])
    if test "x$fluidsynth_available" = "xyes"; then
      enable_amidiplug=yes
      ap_disable_timidity=yes
    else
      enable_amidiplug=no
      AC_MSG_WARN([*** Neither ALSA or FluidSynth software synth (>= 1.0.6) have been found, so amidi-plug won't be compiled unless you explicitly request it with --enable-amidiplug ***])
    fi
  fi
fi

if test "x$enable_amidiplug" = "xyes"; then
  INPUT_PLUGINS="$INPUT_PLUGINS amidi-plug"
  PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.6.0],,)
  PKG_CHECK_MODULES(GMODULE, [gmodule-2.0 >= 2.6.0],,)
  AMIDIPLUGDATADIR=${libdir}/amidi-plug/backends
  AC_SUBST(AMIDIPLUGDATADIR)
  if test "x$enable_amidiplug_alsa" = "xauto" -o "x$enable_amidiplug_alsa" = "xyes"; then
    if test "x$alsalib_available" = "xcheck"; then
      PKG_CHECK_MODULES(ALSA, [alsa >= 1.0],
        [alsalib_available=yes],[alsalib_available=no])
    fi
    if test "x$alsalib_available" = "xyes"; then
      AMIDIPLUG_BACKENDS="$AMIDIPLUG_BACKENDS backend-alsa"
      enable_amidiplug_alsa=yes
    else
      if test "x$enable_amidiplug_alsa" = "xyes"; then
        AC_MSG_ERROR([Cannot find ALSA development files (ver >= 1.0), but compilation of AMIDI-Plug ALSA backend has been explicitly requested; please install ALSA dev files and run configure again])
        enable_amidiplug_alsa=no
      else
        enable_amidiplug_alsa=no
      fi
    fi
  fi
  if test "x$enable_amidiplug_fluidsynth" = "xauto" -o "x$enable_amidiplug_fluidsynth" = "xyes"; then
    if test "x$fluidsynth_available" != "xyes" -a "x$fluidsynth_available" != "xno"; then
      PKG_CHECK_MODULES(FLUIDSYNTH, [fluidsynth >= 1.0.6],
        [fluidsynth_available=yes],[fluidsynth_available=no])
    fi
    if test "x$fluidsynth_available" = "xyes"; then
      AMIDIPLUG_BACKENDS="$AMIDIPLUG_BACKENDS backend-fluidsynth"
      enable_amidiplug_fluidsynth=yes
    else
      if test "x$enable_amidiplug_fluidsynth" = "xyes"; then
        AC_MSG_ERROR([Cannot find FluidSynth development files (ver >= 1.0.6), but compilation of AMIDI-Plug FluidSynth backend has been explicitly requested; please install FluidSynth dev files and run configure again])
        enable_amidiplug_fluidsynth=no
      else
        enable_amidiplug_fluidsynth=no
      fi
    fi
  fi
  if test "x$enable_amidiplug_dummy" = "xauto" -o "x$enable_amidiplug_dummy" = "xyes"; then
    AMIDIPLUG_BACKENDS="$AMIDIPLUG_BACKENDS backend-dummy"
    enable_amidiplug_dummy=yes
  fi
  AC_SUBST(AMIDIPLUG_BACKENDS)
fi

if test "x$enable_amidiplug" = "xyes"; then
  INPUT_PLUGINS="$INPUT_PLUGINS amidi-plug"
fi


dnl *** TiMidity input plugin (note: should appear somewhere after the amidi-plug check)

AC_ARG_ENABLE(timidity,
[  --disable-timidity      disable timidity plugin. (default=enabled)],
[enable_timidity=$enableval],
[
  if test "x$ap_disable_timidity" = "xyes"; then
    enable_timidity=no
  else
    enable_timidity=yes
  fi
]
)

if test "$enable_timidity" = "yes"; then
	INPUT_PLUGINS="$INPUT_PLUGINS timidity"
fi

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

AC_CHECK_FUNCS([mkdtemp getmntinfo strtoul lrintf])


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
    CFLAGS="$CFLAGS -Wall -Wno-strict-aliasing -Wno-unused-parameter -Wpointer-arith -finline-functions -ffast-math"
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
        OUTPUT_PLUGINS="$OUTPUT_PLUGINS sun"
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

dnl *** Scrobbler ***

scrobbler="yes"

dnl Checks for libmusicbrainz
PKG_CHECK_MODULES(MUSICBRAINZ, [libmusicbrainz >= 2.0.0], , [scrobbler="no"])

dnl libcurl check

my_cv_curl_vers=NONE
dnl check is the plain-text version of the required version
check="7.9.7"
dnl check_hex must be UPPERCASE if any hex letters are present
check_hex="070907"

AC_MSG_CHECKING([for curl >= $check])

PKG_CHECK_MODULES(CURL, [libcurl >= 7.9.7], , [scrobbler="no"])

if test "x$scrobbler" = "xyes"; then
	GENERAL_PLUGINS="$GENERAL_PLUGINS scrobbler"
fi

AC_SUBST(CURL_CFLAGS)
AC_SUBST(CURL_LIBS)
AC_SUBST(MUSICBRAINZ_LIBS)

dnl *** End of Scrobbler checks ***

dnl *** libparanormal checks

AC_ARG_ENABLE(paranormal,
[  --disable-paranormal    disable paranormal vis plugin. (default=enabled)],
[have_paranormal=$enableval],
[
  if test "x$ap_disable_paranormal" = "xyes"; then
    have_paranormal=no
    AC_MSG_RESULT([*** paranormal plugin disabled by request ***])
  else
    have_paranormal=yes
    AM_PATH_SDL(1.2.5, , [
	AC_MSG_WARN([*** SDL is not installed. Paranormal will not be built. ***])
	have_paranormal="no"
    ])

    AM_PATH_XML2(2.0.0, , [
	AC_MSG_WARN([*** libxml2 is not installed. Paranormal will not be built. ***])
	have_paranormal="no"
    ])
  fi
]
)

if test "$have_paranormal" = "yes"; then
	VISUALIZATION_PLUGINS="$VISUALIZATION_PLUGINS paranormal"
fi

dnl *** End of all plugin checks ***

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
else
    pluginsubs="\\\"Output\\\",\\\"Input\\\",\\\"Effect\\\",\\\"General\\\",\\\"Visualization\\\",\\\"Container\\\""
    INPUT_PLUGIN_DIR=Input
    OUTPUT_PLUGIN_DIR=Output
    EFFECT_PLUGIN_DIR=Effect
    GENERAL_PLUGIN_DIR=General
    VISUALIZATION_PLUGIN_DIR=Visualization
    CONTAINER_PLUGIN_DIR=Container
fi

AC_SUBST(INPUT_PLUGIN_DIR)
AC_SUBST(OUTPUT_PLUGIN_DIR)
AC_SUBST(EFFECT_PLUGIN_DIR)
AC_SUBST(GENERAL_PLUGIN_DIR)
AC_SUBST(VISUALIZATION_PLUGIN_DIR)
AC_SUBST(CONTAINER_PLUGIN_DIR)
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

AC_SUBST(beepdir)
AC_SUBST(plugindir)

AC_SUBST(EFFECT_PLUGINS)
AC_SUBST(GENERAL_PLUGINS)
AC_SUBST(INPUT_PLUGINS)
AC_SUBST(OUTPUT_PLUGINS)
AC_SUBST(VISUALIZATION_PLUGINS)
AC_SUBST(CONTAINER_PLUGINS)

AC_CONFIG_FILES([
        audacious.1
        audacious.spec
        audacious.pc
        audacious/audacious.desktop
	audacious/images/Makefile
        libaudacious/Makefile
        libguess/Makefile
	Plugins/Makefile
        Plugins/Output/Makefile
        Plugins/Output/OSS/Makefile
        Plugins/Output/esd/Makefile
        Plugins/Output/alsa/Makefile
	Plugins/Output/jack/Makefile
	Plugins/Output/arts/Makefile
	Plugins/Output/arts/arts_helper/Makefile
	Plugins/Output/disk_writer/Makefile
	Plugins/Output/sun/Makefile
        Plugins/Input/Makefile
        Plugins/Input/mpg123/Makefile
	Plugins/Input/aac/Makefile
	Plugins/Input/aac/mp4ff/Makefile
	Plugins/Input/aac/libfaad2/Makefile
	Plugins/Input/aac/src/Makefile
	Plugins/Input/modplug/Makefile
	Plugins/Input/modplug/archive/Makefile
	Plugins/Input/modplug/gui/Makefile
	Plugins/Input/musepack/Makefile
	Plugins/Input/tonegen/Makefile
        Plugins/Input/vorbis/Makefile
        Plugins/Input/cdaudio/Makefile
        Plugins/Input/wav/Makefile
	Plugins/Input/flac/Makefile
	Plugins/Input/flac/plugin_common/Makefile
	Plugins/Input/sid/Makefile
	Plugins/Input/console/Makefile
	Plugins/Input/wma/Makefile
	Plugins/Input/wma/libffwma/Makefile
	Plugins/Input/timidity/Makefile
	Plugins/Input/timidity/libtimidity/Makefile
	Plugins/Input/timidity/src/Makefile
	Plugins/Input/sexypsf/Makefile
	Plugins/Input/adplug/Makefile
	Plugins/Input/adplug/core/Makefile
	Plugins/Input/amidi-plug/Makefile
	Plugins/Input/amidi-plug/pcfg/Makefile
	Plugins/Input/amidi-plug/backend-alsa/Makefile
	Plugins/Input/amidi-plug/backend-fluidsynth/Makefile
	Plugins/Input/amidi-plug/backend-dummy/Makefile
	Plugins/Visualization/Makefile
	Plugins/Visualization/blur_scope/Makefile
	Plugins/Visualization/rocklight/Makefile
	Plugins/General/Makefile
	Plugins/General/song_change/Makefile
	Plugins/General/lirc/Makefile
	Plugins/General/scrobbler/Makefile
	Plugins/Effect/Makefile
	Plugins/Effect/audiocompress/Makefile
	Plugins/Effect/ladspa/Makefile
	Plugins/Effect/stereo_plugin/Makefile
	Plugins/Effect/voice_removal/Makefile
	Plugins/Container/Makefile
        po/Makefile.in
        intl/Makefile
        icons/Makefile
        skin/Makefile
	mk/rules.mk
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
echo "  Jack Audio Connection Kit (jack):       $have_jack"
echo "  Analog Realtime Synthesizer (arts):     $have_arts"
echo "  BSD/SUN audio output (sun):             $have_sun"
echo
echo "  Input Plugins"
echo "  -------------"
echo "  MPEG 1/2/3 (mpg123):                    $enable_mp3"
echo "  MPEG 4 Audio (AAC):                     $enable_aac"
echo "  Windows Media Audio (wma):              $enable_wma"
echo "  .mpc playback (musepack):               $enable_musepack"
echo "  Module decoder (modplug):               $have_modplug"
echo "  MIDI modular plugin (amidi-plug):       $enable_amidiplug"
echo "    -> ALSA backend:                      $enable_amidiplug_alsa"
echo "    -> FluidSynth backend:                $enable_amidiplug_fluidsynth"
echo "    -> dummy backend:                     $enable_amidiplug_dummy"
echo "  MIDI to WAVE converter (timidity):      $enable_timidity"
echo "  CD Digital Audio (cdda):                yes"
echo "  Microsoft WAV (wav):                    yes"
echo "    + sndfile extensions:                 $enable_sndfile"
echo "  Tone Generator:                         yes"
echo "  Ogg Vorbis (vorbis):                    $have_oggvorbis"
echo "  Free Lossless Audio Codec (flac):       $have_flac"
echo "  Commodore 64 audio (sid):               $have_sidplay"
echo "  Game music (spc, nsf & gbs):            yes"
echo "  PlayStation audio (sexypsf):            yes"
echo "  AdLib synthesizer (adplug):             $have_adplug"
echo
echo "  General"
echo "  -------"
echo "  Song Change:                            yes"
echo "  LIRC:                                   $have_lirc"
echo "  AudioScrobbler Client:                  $scrobbler"
echo "  libnotify Plugin:                       $have_libnotify"
echo
echo "  Effect"
echo "  ------"
echo "  AudioCompressor (AGC):                  yes"
echo "  LADSPA effects host (ladspa):           yes"
echo "  Voice Removal:                          yes"
echo
echo "  Visualization"
echo "  -------------"
echo "  Blur Scope:                             yes"
echo "  Paranormal Visualization Library:       $have_paranormal"
echo
echo "  Container"
echo "  -------------"
echo "  Winamp PLS playlist format (pls):       yes"
echo "  M3U playlist format (m3u):              yes"
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

if test "x$ap_disable_timidity" = "xyes" && test "x$enable_timidity" = "xno"; then
    echo "Note: an ALSA-supported hardware midi synth or a FluidSynth software synth"
    echo "was detected in configuration; in order to use it, the amidi-plug input"
    echo "plugin will be compiled in place of timidity. If you wish to compile timidity"
    echo "anyway, simply use --enable-timidity in configuration."
    echo ""
fi
