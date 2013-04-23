#!/bin/sh

# compatibility script for automake <= 1.12
# users of automake >= 1.13 should run 'autoreconf' instead

aclocal -I m4
autoheader
autoconf
