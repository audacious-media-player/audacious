#!/bin/sh
set -e

# Set a version for OpenBSD
if test x"$(uname -s)" = x"OpenBSD"; then
	: ${AUTOCONF_VERSION:=2.71}
	: ${AUTOMAKE_VERSION:=1.16}
	export AUTOCONF_VERSION AUTOMAKE_VERSION
fi

aclocal -I m4
autoconf
autoheader
