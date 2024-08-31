#!/bin/sh

if ! test -e .git
then
	echo This script must be run in a Git repository
	exit 1
fi

RELEASENAME=`git describe --tags`

if test -e $RELEASENAME
then
	echo $RELEASENAME already exists, not overwriting
	exit 1
fi

echo "Exporting HEAD to $RELEASENAME ..."
git archive --format=tar --prefix=$RELEASENAME/ HEAD | tar x || exit 1

echo "Running autoreconf ..."
cd $RELEASENAME || exit 1
autoreconf || exit 1
rm -rf .github .gitignore aclocal.m4 autom4te.cache

echo "Building $RELEASENAME.tar.bz2 ..."
cd .. || exit 1
tar cfj $RELEASENAME.tar.bz2 $RELEASENAME || exit 1

echo "Calculating checksum ..."
sha256sum $RELEASENAME.tar.bz2 > $RELEASENAME.sha256sum || exit 1
