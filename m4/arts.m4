# CFLAGS and library paths for aRts
# written 15 December 1999 by Ben Gertzfield <che@debian.org>
# hacked for artsc by Haavard Kvaalen <havardk@xmms.org>

dnl Usage:
dnl AM_PATH_ARTSC([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl
dnl Example:
dnl AM_PATH_ARTSC(0.9.5, , AC_MSG_ERROR([*** ARTSC >= 0.9.5 not installed - please install first ***]))
dnl
dnl Defines ARTSC_CFLAGS, ARTSC_LIBS and ARTSC_VERSION.
dnl

dnl ARTSC_TEST_VERSION(AVAILABLE-VERSION, NEEDED-VERSION [, ACTION-IF-OKAY [, ACTION-IF-NOT-OKAY]])
AC_DEFUN([ARTSC_TEST_VERSION], [

# Determine which version number is greater. Prints 2 to stdout if	
# the second number is greater, 1 if the first number is greater,	
# 0 if the numbers are equal.						
									
# Written 15 December 1999 by Ben Gertzfield <che@debian.org>		
# Revised 15 December 1999 by Jim Monty <monty@primenet.com>		
									
    AC_PROG_AWK
    artsc_got_version=[` $AWK '						\
BEGIN {									\
    print vercmp(ARGV[1], ARGV[2]);					\
}									\
									\
function vercmp(ver1, ver2,    ver1arr, ver2arr,			\
                               ver1len, ver2len,			\
                               ver1int, ver2int, len, i, p) {		\
									\
    ver1len = split(ver1, ver1arr, /\./);				\
    ver2len = split(ver2, ver2arr, /\./);				\
									\
    len = ver1len > ver2len ? ver1len : ver2len;			\
									\
    for (i = 1; i <= len; i++) {					\
        p = 1000 ^ (len - i);						\
        ver1int += ver1arr[i] * p;					\
        ver2int += ver2arr[i] * p;					\
    }									\
									\
    if (ver1int < ver2int)						\
        return 2;							\
    else if (ver1int > ver2int)						\
        return 1;							\
    else								\
        return 0;							\
}' $1 $2`]								

    if test $artsc_got_version -eq 2; then 	# failure
	ifelse([$4], , :, $4)			
    else  					# success!
	ifelse([$3], , :, $3)
    fi
])

AC_DEFUN([AM_PATH_ARTSC],
[
AC_ARG_WITH(artsc-prefix,[  --with-artsc-prefix=PFX  Prefix where aRts is installed (optional)],
	artsc_config_prefix="$withval", artsc_config_prefix="")
AC_ARG_WITH(artsc-exec-prefix,[  --with-artsc-exec-prefix=PFX Exec prefix where aRts is installed (optional)],
	artsc_config_exec_prefix="$withval", artsc_config_exec_prefix="")

if test x$artsc_config_exec_prefix != x; then
    artsc_config_args="$artsc_config_args --exec-prefix=$artsc_config_exec_prefix"
    if test x${ARTSC_CONFIG+set} != xset; then
	ARTSC_CONFIG=$artsc_config_exec_prefix/bin/artsc-config
    fi
fi

if test x$artsc_config_prefix != x; then
    artsc_config_args="$artsc_config_args --prefix=$artsc_config_prefix"
    if test x${ARTSC_CONFIG+set} != xset; then
  	ARTSC_CONFIG=$artsc_config_prefix/bin/artsc-config
    fi
fi

AC_PATH_PROG(ARTSC_CONFIG, artsc-config, no)
min_artsc_version=ifelse([$1], ,0.9.5.1, $1)

if test "$ARTSC_CONFIG" = "no"; then
    no_artsc=yes
else
    ARTSC_CFLAGS=`$ARTSC_CONFIG $artsc_config_args --cflags`
    ARTSC_LIBS=`$ARTSC_CONFIG $artsc_config_args --libs`
    ARTSC_VERSION=`$ARTSC_CONFIG $artsc_config_args --version`

    ARTSC_TEST_VERSION($ARTSC_VERSION, $min_artsc_version, ,no_artsc=version)
fi

AC_MSG_CHECKING(for artsc - version >= $min_artsc_version)

if test "x$no_artsc" = x; then
    AC_MSG_RESULT(yes)
    AC_SUBST(ARTSC_CFLAGS)
    AC_SUBST(ARTSC_LIBS)
    AC_SUBST(ARTSC_VERSION)
    ifelse([$2], , :, [$2])
else
    AC_MSG_RESULT(no)

    if test "$ARTSC_CONFIG" = "no" ; then
	echo "*** The artsc-config script installed by aRts could not be found."
      	echo "*** If aRts was installed in PREFIX, make sure PREFIX/bin is in"
	echo "*** your path, or set the ARTSC_CONFIG environment variable to the"
	echo "*** full path to artsc-config."
    else
	if test "$no_artsc" = "version"; then
	    echo "*** An old version of aRts, $ARTSC_VERSION, was found."
	    echo "*** You need a version of aRts newer than $min_artsc_version."
	    echo "*** The latest version of aRts is available from"
	    echo "*** http://www.arts-project.org/"
	    echo "***"

            echo "*** If you have already installed a sufficiently new version, this error"
            echo "*** probably means that the wrong copy of the artsc-config shell script is"
            echo "*** being found. The easiest way to fix this is to remove the old version"
            echo "*** of aRts, but you can also set the ARTSC_CONFIG environment to point to the"
            echo "*** correct copy of artsc-config. (In this case, you will have to"
            echo "*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf"
            echo "*** so that the correct libraries are found at run-time)"
	fi
    fi
    ARTSC_CFLAGS=""
    ARTSC_LIBS=""
    ifelse([$3], , :, [$3])
fi
])
