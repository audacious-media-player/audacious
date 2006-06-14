#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(audtool, 0.0, [nenolod@nenolod.net])
AC_CONFIG_SRCDIR([audtool.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_ISC_POSIX
AC_PROG_CC
AC_PROG_INSTALL
AC_PATH_PROG(RM, rm)
AC_PATH_PROG(MV, mv)
AC_PATH_PROG(CP, cp)
AC_PATH_PROG(AR, ar)
AC_PATH_PROG(RANLIB, ranlib)
AC_PROG_MAKE_SET

# Checks for libraries.
PKG_CHECK_MODULES(AUDACIOUS, [audacious >= 0.1],
	[],
	[AC_MSG_ERROR([Can't find Audacious development files.])])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([strcasecmp])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
