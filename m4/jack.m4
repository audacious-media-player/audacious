# Configure paths for JACK

dnl AM_PATH_JACK([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for JACK, and define JACK_CFLAGS and JACK_LIBS
dnl
AC_DEFUN([AM_PATH_JACK],
[dnl 
dnl **** Check for Jack sound server ****
dnl
JACK_LIBS=
JACK_CFLAGS=
JACK_EVERYTHINGOK=yes

AC_CHECK_HEADERS(jack/jack.h)
if test "${ac_cv_header_jack_jack_h}" = "no"
then
  AC_MSG_WARN([Could not find jack/jack.h  Install jack headers to build bio2jack])
  JACK_EVERYTHINGOK=no
else
  JACK_CFLAGS="-lpthread -ljack -ldl"
fi

AC_CHECK_LIB(jack, jack_activate, JACK_LIBS="-ljack -ldl")
if test "${ac_cv_lib_jack_jack_activate}" = "no"
then
  AC_MSG_WARN([Could not find jack_activate in libjack.  Ensure that you have libjack installed and that it a current version.])
  JACK_EVERYTHINGOK=no
fi

AC_SUBST(JACK_CFLAGS)
AC_SUBST(JACK_LIBS)

dnl **** Check for libsamplerate necessary for bio2jack ****
PKG_CHECK_MODULES(SAMPLERATE, samplerate >= 0.0.15,
            ac_cv_samplerate=1, ac_cv_samplerate=0)

AC_DEFINE_UNQUOTED([HAVE_SAMPLERATE],${ac_cv_samplerate},
            [Set to 1 if you have libsamplerate.])

dnl Make sure libsamplerate is found, we can't compile without it
if test "${ac_cv_samplerate}" = 0
then
  AC_MSG_WARN([Could not find libsamplerate, necessary for jack output plugin.])
  JACK_EVERYTHINGOK=no
fi

AC_SUBST(SAMPLERATE_CFLAGS)
AC_SUBST(SAMPLERATE_LIBS)

if test "x$JACK_EVERYTHINGOK" = xno; then
  ifelse([$2], , :, [$2])
else
  ifelse([$1], , :, [$1])
fi

])
