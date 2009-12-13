dnl
dnl Copyright (c) 2007 - 2009, Jonathan Schleifer <js@webkeks.org>
dnl
dnl https://webkeks.org/hg/buildsys/
dnl
dnl Permission to use, copy, modify, and/or distribute this software for any
dnl purpose with or without fee is hereby granted, provided that the above
dnl copyright notice and this permission notice is present in all copies.
dnl
dnl THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
dnl AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
dnl IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
dnl ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
dnl LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
dnl CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
dnl SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
dnl INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
dnl CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
dnl ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
dnl POSSIBILITY OF SUCH DAMAGE.
dnl

AC_DEFUN([BUILDSYS_LIB], [
	AC_ARG_ENABLE(shared,
		AS_HELP_STRING([--disable-shared], [don't build shared libraries]))

	AS_IF([test x"$enable_shared" = x"no"],
		[BUILDSYS_STATIC_LIB_ONLY],
		[BUILDSYS_SHARED_LIB])
])

AC_DEFUN([BUILDSYS_PROG_IMPLIB], [
	AC_REQUIRE([AC_CANONICAL_HOST])
	AC_MSG_CHECKING(whether we need an implib)
	case "$host_os" in
		cygwin* | mingw*)
			AC_MSG_RESULT(yes)
			PROG_IMPLIB_NEEDED='yes'
			PROG_IMPLIB_LDFLAGS='-Wl,-export-all-symbols,--out-implib,lib${PROG}.a'
			;;
		*)
			AC_MSG_RESULT(no)
			PROG_IMPLIB_NEEDED='no'
			PROG_IMPLIB_LDFLAGS=''
			;;
	esac

	AC_SUBST(PROG_IMPLIB_NEEDED)
	AC_SUBST(PROG_IMPLIB_LDFLAGS)
])

AC_DEFUN([BUILDSYS_SHARED_LIB], [
	AC_REQUIRE([AC_CANONICAL_HOST])
	AC_MSG_CHECKING(for shared library system)
	case "$host_os" in
		darwin*)
			AC_MSG_RESULT(Darwin)
			LIB_CPPFLAGS='-DPIC'
			LIB_CFLAGS='-fPIC'
			LIB_LDFLAGS='-dynamiclib -flat_namespace'
			LIB_PREFIX='lib'
			LIB_SUFFIX='.dylib'
			LDFLAGS_RPATH='-Wl,-rpath,${libdir}'
			PLUGIN_CPPFLAGS='-DPIC'
			PLUGIN_CFLAGS='-fPIC'
			PLUGIN_LDFLAGS='-bundle -flat_namespace -undefined suppress'
			PLUGIN_SUFFIX='.impl'
			INSTALL_LIB='${INSTALL} -m 755 $$i ${DESTDIR}${libdir}/$${i%.dylib}.${LIB_MAJOR}.${LIB_MINOR}.dylib && ${LN_S} -f $${i%.dylib}.${LIB_MAJOR}.${LIB_MINOR}.dylib ${DESTDIR}${libdir}/$${i%.dylib}.${LIB_MAJOR}.dylib && ${LN_S} -f $${i%.dylib}.${LIB_MAJOR}.${LIB_MINOR}.dylib ${DESTDIR}${libdir}/$$i'
			UNINSTALL_LIB='rm -f ${DESTDIR}${libdir}/$$i ${DESTDIR}${libdir}/$${i%.dylib}.${LIB_MAJOR}.dylib ${DESTDIR}${libdir}/$${i%.dylib}.${LIB_MAJOR}.${LIB_MINOR}.dylib'
			CLEAN_LIB=''
			;;
		solaris*)
			AC_MSG_RESULT(Solaris)
			LIB_CPPFLAGS='-DPIC'
			LIB_CFLAGS='-fPIC'
			LIB_LDFLAGS='-shared -fPIC -Wl,-soname=${LIB}.${LIB_MAJOR}.${LIB_MINOR}'
			LIB_PREFIX='lib'
			LIB_SUFFIX='.so'
			LDFLAGS_RPATH='-Wl,-rpath,${libdir}'
			PLUGIN_CPPFLAGS='-DPIC'
			PLUGIN_CFLAGS='-fPIC'
			PLUGIN_LDFLAGS='-shared -fPIC'
			PLUGIN_SUFFIX='.so'
			INSTALL_LIB='${INSTALL} -m 755 $$i ${DESTDIR}${libdir}/$$i.${LIB_MAJOR}.${LIB_MINOR} && rm -f ${DESTDIR}${libdir}/$$i && ${LN_S} $$i.${LIB_MAJOR}.${LIB_MINOR} ${DESTDIR}${libdir}/$$i'
			UNINSTALL_LIB='rm -f ${DESTDIR}${libdir}/$$i ${DESTDIR}${libdir}/$$i.${LIB_MAJOR}.${LIB_MINOR}'
			CLEAN_LIB=''
			;;
		openbsd* | mirbsd*)
			AC_MSG_RESULT(OpenBSD)
			LIB_CPPFLAGS='-DPIC'
			LIB_CFLAGS='-fPIC'
			LIB_LDFLAGS='-shared -fPIC'
			LIB_PREFIX='lib'
			LIB_SUFFIX='.so.${LIB_MAJOR}.${LIB_MINOR}'
			LDFLAGS_RPATH='-Wl,-rpath,${libdir}'
			PLUGIN_CPPFLAGS='-DPIC'
			PLUGIN_CFLAGS='-fPIC'
			PLUGIN_LDFLAGS='-shared -fPIC'
			PLUGIN_SUFFIX='.so'
			INSTALL_LIB='${INSTALL} -m 755 $$i ${DESTDIR}${libdir}/$$i'
			UNINSTALL_LIB='rm -f ${DESTDIR}${libdir}/$$i'
			CLEAN_LIB=''
			;;
		cygwin* | mingw*)
			AC_MSG_RESULT(Win32)
			LIB_CPPFLAGS='-DPIC'
			LIB_CFLAGS=''
			LIB_LDFLAGS='-shared -Wl,--out-implib,${LIB}.a'
			LIB_PREFIX='lib'
			LIB_SUFFIX='.dll'
			LDFLAGS_RPATH='-Wl,-rpath,${libdir}'
			PLUGIN_CPPFLAGS=''
			PLUGIN_CFLAGS=''
			PLUGIN_LDFLAGS='-shared'
			PLUGIN_SUFFIX='.dll'
			INSTALL_LIB='${MKDIR_P} ${DESTDIR}${bindir} && ${INSTALL} -m 755 $$i ${DESTDIR}${bindir}/$$i && ${INSTALL} -m 755 $$i.a ${DESTDIR}${libdir}/$$i.a'
			UNINSTALL_LIB='rm -f ${DESTDIR}${bindir}/$$i ${DESTDIR}${libdir}/$$i.a'
			CLEAN_LIB='${LIB}.a'
			;;
		*)
			AC_MSG_RESULT(GNU)
			LIB_CPPFLAGS='-DPIC'
			LIB_CFLAGS='-fPIC'
			LIB_LDFLAGS='-shared -fPIC -Wl,-soname=${LIB}.${LIB_MAJOR}'
			LIB_PREFIX='lib'
			LIB_SUFFIX='.so'
			LDFLAGS_RPATH='-Wl,-rpath,${libdir}'
			PLUGIN_CPPFLAGS='-DPIC'
			PLUGIN_CFLAGS='-fPIC'
			PLUGIN_LDFLAGS='-shared -fPIC'
			PLUGIN_SUFFIX='.so'
			INSTALL_LIB='${INSTALL} -m 755 $$i ${DESTDIR}${libdir}/$$i.${LIB_MAJOR}.${LIB_MINOR}.0 && ${LN_S} -f $$i.${LIB_MAJOR}.${LIB_MINOR}.0 ${DESTDIR}${libdir}/$$i.${LIB_MAJOR} && ${LN_S} -f $$i.${LIB_MAJOR}.${LIB_MINOR}.0 ${DESTDIR}${libdir}/$$i'
			UNINSTALL_LIB='rm -f ${DESTDIR}${libdir}/$$i ${DESTDIR}${libdir}/$$i.${LIB_MAJOR} ${DESTDIR}${libdir}/$$i.${LIB_MAJOR}.${LIB_MINOR}.0'
			CLEAN_LIB=''
			;;
	esac

	AC_SUBST(LIB_CPPFLAGS)
	AC_SUBST(LIB_CFLAGS)
	AC_SUBST(LIB_LDFLAGS)
	AC_SUBST(LIB_PREFIX)
	AC_SUBST(LIB_SUFFIX)
	AC_SUBST(LDFLAGS_RPATH)
	AC_SUBST(PLUGIN_CPPFLAGS)
	AC_SUBST(PLUGIN_CFLAGS)
	AC_SUBST(PLUGIN_LDFLAGS)
	AC_SUBST(PLUGIN_SUFFIX)
	AC_SUBST(INSTALL_LIB)
	AC_SUBST(UNINSTALL_LIB)
	AC_SUBST(CLEAN_LIB)
])

AC_DEFUN([BUILDSYS_STATIC_LIB_ONLY], [
	AC_REQUIRE([AC_PROG_RANLIB])
	AC_PATH_TOOL(AR, ar)

	LIB_CPPFLAGS=''
	LIB_CFLAGS=''
	LIB_LDFLAGS=''
	LIB_PREFIX='lib'
	LIB_SUFFIX='.a'
	LDFLAGS_RPATH=''
	INSTALL_LIB='${INSTALL} -m 644 $$i ${DESTDIR}${libdir}/$$i'
	UNINSTALL_LIB='rm -f ${DESTDIR}${libdir}/$$i'
	CLEAN_LIB=''

	AC_SUBST(LIB_CPPFLAGS)
	AC_SUBST(LIB_CFLAGS)
	AC_SUBST(LIB_LDFLAGS)
	AC_SUBST(LIB_PREFIX)
	AC_SUBST(LIB_SUFFIX)
	AC_SUBST(LDFLAGS_RPATH)
	AC_SUBST(INSTALL_LIB)
	AC_SUBST(UNINSTALL_LIB)
	AC_SUBST(CLEAN_LIB)
])

AC_DEFUN([BUILDSYS_TOUCH_DEPS], [
	${as_echo:="echo"} "${as_me:="configure"}: touching .deps files"
	for i in $(find . -name Makefile); do
		DEPSFILE="$(dirname $i)/.deps"
		test -f "$DEPSFILE" && rm "$DEPSFILE"
		touch -t 0001010000 "$DEPSFILE"
	done
])
