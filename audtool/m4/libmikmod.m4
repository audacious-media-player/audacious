# Configure paths for libmikmod
#
# Derived from libmikmod.m4 (Owen Taylor 97-11-3)
#

dnl AM_PATH_LIBMIKMOD([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]]])
dnl Test for libmikmod, and define LIBMIKMOD_CFLAGS, LIBMIKMOD_LIBS and
dnl LIBMIKMOD_LDADD
dnl
AC_DEFUN([AM_PATH_LIBMIKMOD],
[dnl 
dnl Get the cflags and libraries from the libmikmod-config script
dnl
AC_ARG_WITH(libmikmod-prefix,[  --with-libmikmod-prefix=PFX   Prefix where libmikmod is installed (optional)],
            libmikmod_config_prefix="$withval", libmikmod_config_prefix="")
AC_ARG_WITH(libmikmod-exec-prefix,[  --with-libmikmod-exec-prefix=PFX Exec prefix where libmikmod is installed (optional)],
            libmikmod_config_exec_prefix="$withval", libmikmod_config_exec_prefix="")

  if test x$libmikmod_config_exec_prefix != x ; then
     libmikmod_config_args="$libmikmod_config_args --exec-prefix=$libmikmod_config_exec_prefix"
     if test x${LIBMIKMOD_CONFIG+set} != xset ; then
        LIBMIKMOD_CONFIG=$libmikmod_config_exec_prefix/bin/libmikmod-config
     fi
  fi
  if test x$libmikmod_config_prefix != x ; then
     libmikmod_config_args="$libmikmod_config_args --prefix=$libmikmod_config_prefix"
     if test x${LIBMIKMOD_CONFIG+set} != xset ; then
        LIBMIKMOD_CONFIG=$libmikmod_config_prefix/bin/libmikmod-config
     fi
  fi

  AC_PATH_PROG(LIBMIKMOD_CONFIG, libmikmod-config, no)
  min_libmikmod_version=ifelse([$1], ,3.1.5,$1)
  AC_MSG_CHECKING(for libmikmod - version >= $min_libmikmod_version)
  no_libmikmod=""
  if test "$LIBMIKMOD_CONFIG" = "no" ; then
    no_libmikmod=yes
  else
    LIBMIKMOD_CFLAGS=`$LIBMIKMOD_CONFIG $libmikmod_config_args --cflags`
    LIBMIKMOD_LIBS=`$LIBMIKMOD_CONFIG $libmikmod_config_args --libs`
    LIBMIKMOD_LDADD=`$LIBMIKMOD_CONFIG $libmikmod_config_args --ldadd`
    libmikmod_config_major_version=`$LIBMIKMOD_CONFIG $libmikmod_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\1/'`
    libmikmod_config_minor_version=`$LIBMIKMOD_CONFIG $libmikmod_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\2/'`
    libmikmod_config_micro_version=`$LIBMIKMOD_CONFIG $libmikmod_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\3/'`
  fi
  if test "x$no_libmikmod" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$LIBMIKMOD_CONFIG" = "no" ; then
       echo "*** The libmikmod-config script installed by libmikmod could not be found"
       echo "*** If libmikmod was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the LIBMIKMOD_CONFIG environment variable to the"
       echo "*** full path to libmikmod-config."
     fi
     LIBMIKMOD_CFLAGS=""
     LIBMIKMOD_LIBS=""
     LIBMIKMOD_LDADD=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(LIBMIKMOD_CFLAGS)
  AC_SUBST(LIBMIKMOD_LIBS)
  AC_SUBST(LIBMIKMOD_LDADD)
  rm -f conf.mikmodtest
])

AC_DEFUN([AC_FIND_FILE],
    [
	$3=NO
	for i in $2;
	do
	  for j in $1;
	  do
	    if test -r "$i/$j"; then
		  $3=$i
		  break 2
		fi
	  done
	done
    ]
)

