#!/bin/sh
# mkrelease.sh: Creates a release suitable for distfiles.atheme.org.
#
# Copyright (c) 2007, 2011 atheme.org
#
# Permission to use, copy, modify, and/or distribute this software for
# any purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

if [ "x$1" = "x" ]; then
	echo "usage: $0 releasename [--automatic]"
	exit
else
	RELEASENAME="$1"
fi

if [ "x$2" = "x--automatic" ]; then
	AUTOMATIC="yes"
fi

TIP=`git log -1 --pretty=oneline | cut -d" " -f1`
GITDIR=`git rev-parse --show-toplevel`

WRKDIR=`pwd`

if [ -d $RELEASENAME ]; then
	echo "Deleting previous release named $RELEASENAME."
	rm -rf $WRKDIR/$RELEASENAME/
fi

echo "Making release named $RELEASENAME (tip $TIP)"

echo
echo "Building root: $RELEASENAME/"
cd $GITDIR || exit 1
git archive --format=tar --prefix=$RELEASENAME/ HEAD | gzip >$WRKDIR/$RELEASENAME-working.tar.gz || exit 1
cd $WRKDIR || exit 1
tar -xzvf $RELEASENAME-working.tar.gz || exit 1
cd $RELEASENAME || exit 1
rm -rf .gitignore
rm -rf scripts
sh autogen.sh
rm -rf autogen.sh autom4te.cache

# Run application specific instructions here.
if [ -x "$GITDIR/scripts/application.sh" ]; then
	source $GITDIR/scripts/application.sh
fi

cd ..

echo "Building $RELEASENAME.tar.gz from $RELEASENAME/"
tar zcf $RELEASENAME.tar.gz $RELEASENAME/

echo "Building $RELEASENAME.tar.bz2 from $RELEASENAME/"
tar jcf $RELEASENAME.tar.bz2 $RELEASENAME/

rm $RELEASENAME-working.tar.gz
