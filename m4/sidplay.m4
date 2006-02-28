# Configure paths for SID support

dnl -------------------------------------------------------------------------
dnl Try to find a file (or one of more files in a list of dirs).
dnl -------------------------------------------------------------------------

AC_DEFUN([SIDPLAY_FIND_FILE],
[
$3=""
for i in $2; do
for j in $1; do
if test -r "$i/$j"; then
	$3=$i
	break 2
	fi
done
done
])

AC_DEFUN([SIDPLAY_TRY_LINK_SAVE],
[
    xs_cxx_save="$CXX"
    xs_cxxflags_save="$CXXFLAGS"
    xs_ldflags_save="$LDFLAGS"
    xs_libs_save="$LIBS"
])


AC_DEFUN([SIDPLAY_TRY_LINK_RESTORE],
[
    CXX="$xs_cxx_save"
    CXXFLAGS="$xs_cxxflags_save"
    LDFLAGS="$xs_ldflags_save"
    LIBS="$xs_libs_save"
])


dnl -------------------------------------------------------------------------
dnl Try to find SIDPLAY library and header files.
dnl $xs_have_sidplay1 will be "yes" or "no"
dnl @SIDPLAY1_LDADD@ will be substituted with linker parameters
dnl @SIDPLAY1_INCLUDES@ will be substituted with compiler parameters
dnl -------------------------------------------------------------------------

AC_DEFUN([XS_PATH_LIBSIDPLAY1],
[
    AC_MSG_CHECKING([for working SIDPlay1 library and headers])

    AC_LANG_PUSH([C++])
    
    # Use include path given by user (if any).
    if test -n "$xs_sidplay1_includes"; then
        xs_sidplay1_cxxflags="-I$xs_sidplay1_includes"
    else
        xs_sidplay1_cxxflags=""
    fi

    # Use library path given by user (if any).
    if test -n "$xs_sidplay1_library"; then
        xs_sidplay1_ldflags="-L$xs_sidplay1_library"
    else
        xs_sidplay1_ldflags=""
    fi

    AC_CACHE_VAL(xs_cv_have_sidplay1,
    [
        # Run test compilation with either standard search path
        # or user-defined paths.
        SIDPLAY_TRY_LIBSIDPLAY1
        if test "$xs_sidplay1_works" = yes; then
          xs_cv_have_sidplay1="xs_have_sidplay1=yes  \
            xs_sidplay1_cxxflags=\"$xs_sidplay1_cxxflags\"  \
            xs_sidplay1_ldflags=\"$xs_sidplay1_ldflags\"  "
        else
            SIDPLAY_FIND_LIBSIDPLAY1        
        fi
    ])

    eval "$xs_cv_have_sidplay1"

    if test "$xs_have_sidplay1" = yes; then
        if test -n "$xs_sidplay1_cxxflags" || test -n "$xs_sidplay1_ldflags"; then
            AC_MSG_RESULT([$xs_sidplay1_cxxflags $xs_sidplay1_ldflags])
        else
            AC_MSG_RESULT([yes])
        fi

    SIDPLAY1_LDADD="$xs_sidplay1_ldflags -lsidplay"
    SIDPLAY1_INCLUDES="$xs_sidplay1_cxxflags"
    AC_SUBST([SIDPLAY1_LDADD])
    AC_SUBST([SIDPLAY1_INCLUDES])
    else
        AC_MSG_RESULT([no])
    fi

    AC_LANG_POP([C++])
])

dnl Functions used by XS_PATH_LIBSIDPLAY1.

AC_DEFUN([SIDPLAY_FIND_LIBSIDPLAY1],
[
    # Search common locations where header files might be stored.
   xs_sidplay1_incdirs="$xs_sidplay1_includes /usr/include /usr/local/include /usr/lib/sidplay/include /usr/local/lib/sidplay/include /opt/sfw/include /opt/csw/include"
    SIDPLAY_FIND_FILE(sidplay/sidtune.h, $xs_sidplay1_incdirs, xs_sidplay1_includes)

    # Search common locations where library might be stored.
    xs_sidplay1_libdirs="$xs_sidplay1_library /usr/lib /usr/lib/sidplay /usr/local/lib/sidplay /opt/sfw/lib /opt/csw/lib"
    SIDPLAY_FIND_FILE(libsidplay.a libsidplay.so libsidplay.so.1 libsidplay.so.1.36 libsidplay.so.1.37,
                 $xs_sidplay1_libdirs, xs_sidplay1_library)

    if test -z "$xs_sidplay1_includes" || test -z "$xs_sidplay1_library"; then
        xs_cv_have_sidplay1="xs_have_sidplay1=no  \
          xs_sidplay1_ldflags=\"\" xs_sidplay1_cxxflags=\"\"  "
    else
        # Test compilation with found paths.
        xs_sidplay1_ldflags="-L$xs_sidplay1_library"
        xs_sidplay1_cxxflags="-I$xs_sidplay1_includes"
        SIDPLAY_TRY_LIBSIDPLAY1
        xs_cv_have_sidplay1="xs_have_sidplay1=$xs_sidplay1_works  \
          xs_sidplay1_ldflags=\"$xs_sidplay1_ldflags\"  \
          xs_sidplay1_cxxflags=\"$xs_sidplay1_cxxflags\"  "
    fi
])

AC_DEFUN([SIDPLAY_TRY_LIBSIDPLAY1],
[
    SIDPLAY_TRY_LINK_SAVE

    CXXFLAGS="$CXXFLAGS $xs_sidplay1_cxxflags"
    LDFLAGS="$LDFLAGS $xs_sidplay1_ldflags"
    LIBS="$LIBS -lsidplay"

    AC_LINK_IFELSE([AC_LANG_PROGRAM(
        [[#include <sidplay/sidtune.h>]],
        [[sidTune* myTest = new sidTune(0);]])],
        [xs_sidplay1_works=yes],
        [xs_sidplay1_works=no]
    )

    SIDPLAY_TRY_LINK_RESTORE
])





dnl -------------------------------------------------------------------------
dnl Try to find SIDPLAY2 library and header files.
dnl $xs_have_sidplay2 will be "yes" or "no"
dnl @SIDPLAY_LDADD@ will be substituted with linker parameters
dnl @SIDPLAY_INCLUDES@ will be substituted with compiler parameters
dnl -------------------------------------------------------------------------
AC_DEFUN([XS_PATH_LIBSIDPLAY2],
[
    AC_MSG_CHECKING([for working SIDPlay2 library and headers])

    AC_LANG_PUSH([C++])
    
    AC_PATH_PROG([PKG_CONFIG], [pkg-config])
    if test -n "$PKG_CONFIG" && $PKG_CONFIG --atleast-version $LIBSIDPLAY2_REQUIRED_VERSION libsidplay2; then
        xs_pkgcfg_knows=yes
    else
        xs_pkgcfg_knows=no
    fi

    # Derive sidbuilders path from libsidplay2 root.
    if test -n "$xs_sidplay2_library"; then
        xs_sidplay2_builders="$xs_sidplay2_library/sidplay/builders"
    elif test "$xs_pkgcfg_knows" = yes ; then
        xs_sidplay2_builders=`$PKG_CONFIG --variable=builders libsidplay2`
    fi

    AC_CACHE_VAL(xs_cv_have_sidplay2,
    [
        # Run test compilation with either standard search path
        # or user-defined paths.
        xs_sidplay2_ldadd="-lsidplay2"
        SIDPLAY_TRY_LIBSIDPLAY2
        if test "$xs_sidplay2_works" = yes; then
          xs_cv_have_sidplay2="xs_have_sidplay2=yes  \
            xs_sidplay2_cxxflags=\"$xs_sidplay2_cxxflags\"  \
            xs_sidplay2_ldadd=\"$xs_sidplay2_ldadd\"  \
            xs_sidplay2_builders=\"$xs_sidplay2_builders\"  "
        else
            SIDPLAY_FIND_LIBSIDPLAY2
        fi
    ])
    eval "$xs_cv_have_sidplay2"
    if test "$xs_have_sidplay2" = yes; then
        if test -n "$xs_sidplay2_cxxflags" || test -n "$xs_sidplay2_ldadd"; then
            AC_MSG_RESULT([$xs_sidplay2_cxxflags $xs_sidplay2_ldadd])
        else
            AC_MSG_RESULT([yes])
        fi

    SIDPLAY2_LDADD="$xs_sidplay2_ldadd"
    SIDPLAY2_INCLUDES="$xs_sidplay2_cxxflags"
    AC_SUBST([SIDPLAY2_LDADD])
    AC_SUBST([SIDPLAY2_INCLUDES])
    else
        AC_MSG_RESULT([no])
    fi

    AC_LANG_POP([C++])
])


dnl Functions used by XS_PATH_LIBSIDPLAY2.
AC_DEFUN([SIDPLAY_FIND_LIBSIDPLAY2],
[
    # See whether user didn't provide paths.
    if test -z "$xs_sidplay2_includes"; then
        if test "$xs_pkgcfg_knows" = yes ; then
            xs_sidplay2_includes=`$PKG_CONFIG --variable=includedir libsidplay2`
            xs_sidplay2_cxxflags=`$PKG_CONFIG --cflags libsidplay2`
        else
            # Search common locations where header files might be stored.
            xs_sidplay2_incdirs="$xs_sidplay2_includes $xs_sidplay2_includes/include /usr/include /usr/local/include /usr/lib/sidplay/include /usr/local/lib/sidplay/include /opt/sfw/include /opt/csw/include"
            SIDPLAY_FIND_FILE(sidplay/sidplay2.h,$xs_sidplay2_incdirs,xs_sidplay2_includes)
            xs_sidplay2_cxxflags="-I$xs_sidplay2_includes"
        fi
    else
        xs_sidplay2_cxxflags="-I$xs_sidplay2_includes"
    fi
    if test -z "$xs_sidplay2_library"; then
        if test "$xs_pkgcfg_knows" = yes ; then
            xs_sidplay2_library=`$PKG_CONFIG --variable=libdir libsidplay2`
            xs_sidplay2_ldadd=`$PKG_CONFIG --libs libsidplay2`
            xs_sidplay2_builders=`$PKG_CONFIG --variable=builders libsidplay2`
        else
            # Search common locations where library might be stored.
            xs_sidplay2_libdirs="$xs_sidplay2_library $xs_sidplay2_library/lib $xs_sidplay2_library/src /usr/lib /usr/lib/sidplay /usr/local/lib/sidplay /opt/sfw/lib /opt/csw/lib"
            SIDPLAY_FIND_FILE(libsidplay2.la,$xs_sidplay2_libdirs,xs_sidplay2_library)
            xs_sidplay2_ldadd="-L$xs_sidplay2_library -lsidplay2"
            xs_sidplay2_builders="$xs_sidplay2_library/sidplay/builders"
        fi
    else
        xs_sidplay2_ldadd="-L$xs_sidplay2_library -lsidplay2"
    fi
    if test -z "$xs_sidplay2_includes" || test -z "$xs_sidplay2_library"; then
        xs_cv_have_sidplay2="xs_have_sidplay2=no \
          xs_sidplay2_ldadd=\"\" xs_sidplay2_cxxflags=\"\" \
          xs_sidplay2_builders=\"\" "
    else
        # Test compilation with found paths.
        xs_sidplay2_ldadd="-L$xs_sidplay2_library -lsidplay2"
        xs_sidplay2_cxxflags="-I$xs_sidplay2_includes"
        SIDPLAY_TRY_LIBSIDPLAY2
        xs_cv_have_sidplay2="xs_have_sidplay2=$xs_sidplay2_works \
          xs_sidplay2_ldadd=\"$xs_sidplay2_ldadd\" \
          xs_sidplay2_cxxflags=\"$xs_sidplay2_cxxflags\" \
          xs_sidplay2_builders=\"$xs_sidplay2_builders\" "
    fi
])


AC_DEFUN([SIDPLAY_TRY_LIBSIDPLAY2],
[
    SIDPLAY_TRY_LINK_SAVE

    CXXFLAGS="$CXXFLAGS $xs_sidplay2_cxxflags -DHAVE_UNIX"
    LDFLAGS="$LDFLAGS $xs_sidplay2_ldadd"

    AC_LINK_IFELSE([AC_LANG_PROGRAM(
        [[#include <sidplay/sidplay2.h>]],
        [[sidplay2 *myEngine;]])],
        [xs_sidplay2_works=yes],
        [xs_sidplay2_works=no]
    )

    SIDPLAY_TRY_LINK_RESTORE
])


dnl -------------------------------------------------------------------------
dnl Find libsidplay2 builders (sidbuilders) dir.
dnl @BUILDERS_INCLUDES@
dnl @BUILDERS_LDFLAGS@
dnl -------------------------------------------------------------------------
AC_DEFUN([BUILDERS_FIND],
[
    AC_MSG_CHECKING([for SIDPlay2 builders directory])

    AC_LANG_PUSH([C++])

    AC_REQUIRE([XS_PATH_LIBSIDPLAY2])

    dnl Be pessimistic.
    builders_available=no

    dnl Sidbuilder headers are included with "builders" prefix.
    builders_includedir=$xs_sidplay2_includes
    builders_libdir=$xs_sidplay2_builders

    dnl If libsidplay2 is in standard library search path, we need
    dnl to get an argument whether /usr, /usr/local, etc. Else we
    dnl can only use ${libdir}/sidplay/builders, but then are
    dnl unable to check whether files exist as long as ${exec_prefix}
    dnl is not defined in the configure script. So, this is a bit
    dnl ugly, but a satisfactory fallback default for those who
    dnl define ${prefix} and ${exec_prefix}.
    if test -z $builders_libdir; then
        eval "builders_libdir=$libdir/sidplay/builders"
    fi

    AC_ARG_WITH(sidbuilders,
        [  --with-sidbuilders=DIR  what the SIDPlay2 builders install PREFIX is],
        [builders_includedir="$withval/include"
         builders_libdir="$withval/lib/sidplay/builders"])

    AC_ARG_WITH(builders-inc,
        [  --with-builders-inc=DIR where the SIDPlay2 builders headers are located],
        [builders_includedir="$withval"])

    AC_ARG_WITH(builders-lib,
        [  --with-builders-lib=DIR where the SIDPlay2 builders libraries are installed],
        [builders_libdir="$withval"])
    
    if test -n "$builders_includedir"; then
        BUILDERS_INCLUDES="-I$builders_includedir"
    fi

    if test -n "$builders_libdir"; then
        BUILDERS_LDFLAGS="-L$builders_libdir"
    fi

    if test -d $builders_libdir; then
        xs_have_sidbuilders_dir=yes
        AC_MSG_RESULT([$builders_libdir])
    else
        xs_have_sidbuilders_dir=no
        AC_MSG_RESULT([$xs_have_sidbuilders_dir])
        AC_MSG_ERROR([$builders_libdir not found!
Check --help on how to specify SIDPlay2 and/or builders library and
header path, or set --exec-prefix to the same prefix as your installation
of libsidplay2.
        ])
    fi

    AC_SUBST([BUILDERS_INCLUDES])
    AC_SUBST([BUILDERS_LDFLAGS])

    AC_LANG_POP([C++])
])


dnl -------------------------------------------------------------------------
dnl Test for working reSID builder.
dnl sets $(RESID_LDADD), substitutes @RESID_LDADD@
dnl -------------------------------------------------------------------------
AC_DEFUN([BUILDERS_FIND_RESID],
[
    AC_MSG_CHECKING([for reSID builder module])
    AC_LANG_PUSH([C++])
    SIDPLAY_TRY_LINK_SAVE
    
    CXXFLAGS="$CXXFLAGS $BUILDERS_INCLUDES"
    LDFLAGS="$LDFLAGS $BUILDERS_LDFLAGS"
    LIBS="$LIBS -lresid-builder"

    AC_LINK_IFELSE([AC_LANG_PROGRAM(
        [[#include <sidplay/builders/resid.h>]],
        [[ReSIDBuilder *sid;]])],
        [builders_work=yes],
        [builders_work=no]
    )

    SIDPLAY_TRY_LINK_RESTORE

    if test "$builders_work" = yes; then
        builders_available=yes
        xs_builders="reSID $xs_builders"
        AC_DEFINE([HAVE_RESID_BUILDER], [], [resid builder])
        RESID_LDADD="-lresid-builder"
    fi
    AC_MSG_RESULT($builders_work)
    AC_SUBST([RESID_LDADD])
    AC_LANG_POP([C++])
])


dnl -------------------------------------------------------------------------
dnl Test for working HardSID builder.
dnl sets $(HARDSID_LDADD), substitutes @HARDSID_LDADD@
dnl -------------------------------------------------------------------------
AC_DEFUN([BUILDERS_FIND_HARDSID],
[
    AC_MSG_CHECKING([for HardSID builder module])
    AC_LANG_PUSH([C++])
    SIDPLAY_TRY_LINK_SAVE

    CXXFLAGS="$CXXFLAGS $BUILDERS_INCLUDES"
    LDFLAGS="$LDFLAGS $BUILDERS_LDFLAGS"
    LIBS="$LIBS -lhardsid-builder"

    AC_LINK_IFELSE([AC_LANG_PROGRAM(
        [[#include <sidplay/builders/hardsid.h>]],
        [[HardSID *sid;]])],
        [builders_work=yes],
        [builders_work=no]
    )

    SIDPLAY_TRY_LINK_RESTORE

    if test "$builders_work" = yes; then
        builders_available=yes
        xs_builders="HardSID $xs_builders"
        AC_DEFINE([HAVE_HARDSID_BUILDER], [], [harsid builder])
        HARDSID_LDADD="-lhardsid-builder"
    fi
    AC_MSG_RESULT($builders_work)
    AC_SUBST([HARDSID_LDADD])
    AC_LANG_POP([C++])
])


dnl AM_PATH_SIDPLAY([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for SID libraries, and define SIDPLAY_CFLAGS and SIDPLAY_LIBS
dnl
AC_DEFUN([AM_PATH_SIDPLAY],
[
dnl 
dnl Get the cflags and libraries
dnl

dnl ***
dnl *** libSIDPlay 1 options
dnl ***
AC_ARG_WITH(sidplay1,
[  --with-sidplay1=PREFIX   Enable SIDPlay1 with install-PREFIX],
[
if test "x$withval" = xyes; then
xs_sidplay1=yes
xs_sidplay1_library=""
xs_sidplay1_includes=""
else
if test "x$withval" = xno; then
	xs_sidplay1=no
	else
	xs_sidplay1=yes
	xs_sidplay1_includes="$withval/include"
	xs_sidplay1_library="$withval/lib"
	fi
fi
],[
xs_sidplay1=try
xs_sidplay1_library=""
xs_sidplay1_includes=""
])

AC_ARG_WITH(sidplay1-inc,
[  --with-sidplay1-inc=DIR    Where the SIDPlay1 headers are located],
[xs_sidplay1_includes="$withval"],)

AC_ARG_WITH(sidplay1-lib,
[  --with-sidplay1-lib=DIR    Where the SIDPlay1 library is installed],
[xs_sidplay1_library="$withval"],)

dnl ***
dnl *** libSIDPlay 2 options
dnl ***
AC_ARG_WITH(sidplay2,
[  --with-sidplay2=PREFIX   Enable SIDPlay2 with install-PREFIX],
[
if test "x$withval" = xyes; then
xs_sidplay2=yes
xs_sidplay2_library=""
xs_sidplay2_includes=""
else
if test "x$withval" = xno; then
	xs_sidplay2=no
	else
	xs_sidplay2=yes
	xs_sidplay2_includes="$withval/include"
	xs_sidplay2_library="$withval/lib"
	fi
fi
],[
xs_sidplay2=try
xs_sidplay2_library=""
xs_sidplay2_includes=""
])

AC_ARG_WITH(sidplay2-inc,
[  --with-sidplay2-inc=DIR    Where the SIDPlay2 headers are located],
[xs_sidplay2_includes="$withval"],)


AC_ARG_WITH(sidplay2-lib,
[  --with-sidplay2-lib=DIR    Where the SIDPlay2 library is installed],
[xs_sidplay2_library="$withval"],)

dnl ***
dnl *** Determine if libraries are wanted and available
dnl ***
OPT_SIDPLAY1="no"
if test "x$xs_sidplay1" = xtry; then
	XS_PATH_LIBSIDPLAY1
	else
	if test "x$xs_sidplay1" = xyes; then
		XS_PATH_LIBSIDPLAY1
		if test "x$xs_have_sidplay1" = xno; then
		AC_MSG_WARN([libSIDPlay1 library and/or headers were not found!])
		fi
	fi
fi
if test "x$xs_have_sidplay1" = xyes; then
	AC_DEFINE([HAVE_SIDPLAY1],[],[sidplay1 available])
	OPT_SIDPLAY1="yes"
fi

OPT_SIDPLAY2="no"
LIBSIDPLAY2_REQUIRED_VERSION="2.1.0"
if test "x$xs_sidplay2" = xtry; then
	XS_PATH_LIBSIDPLAY2
	else
	if test "x$xs_sidplay2" = xyes; then
		XS_PATH_LIBSIDPLAY2
		if test "x$xs_have_sidplay2" = xno; then
		AC_MSG_WARN([libSIDPlay2 library and/or headers were not found!])
		fi
	fi
fi
if test "x$xs_have_sidplay2" = xyes; then
	AC_DEFINE([HAVE_SIDPLAY2],[],[sidplay2 available])
	OPT_SIDPLAY2="yes"
	BUILDERS_FIND
	BUILDERS_FIND_RESID
	BUILDERS_FIND_HARDSID
	if test "x$builders_available" = xno; then
		AC_MSG_WARN([No builder modules were found in the sidbuilders directory!]);
	fi
fi


dnl ***
dnl *** Check if we have some emulator library available?
dnl ***
if test "x$OPT_SIDPLAY1" = xno; then
if test "x$OPT_SIDPLAY2" = xno; then
SIDPLAY_HAVESOMETHING="no"
fi
fi

if test "x$SIDPLAY_HAVESOMETHING" = xno; then
  ifelse([$2], , :, [$2])
else
  ifelse([$1], , :, [$1])
fi

])
