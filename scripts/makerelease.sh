#!/bin/sh
# mkrelease.sh: Creates a release suitable for distfiles.atheme.org.
#
# Copyright (c) 2007 atheme.org
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

TIP=`hg tip --template "#rev#:#node|short#"`

WRKDIR=`pwd`

if [ -d $RELEASENAME ]; then
	echo "Deleting previous release named $RELEASENAME."
	rm -rf $WRKDIR/$RELEASENAME/
fi

echo "Making release named $RELEASENAME (tip $TIP)"

echo
echo "Building root: $RELEASENAME/"
hg archive $RELEASENAME
cd $RELEASENAME
sh autogen.sh
rm -rf autogen.sh autom4te.cache

# Run application specific instructions here.
if [ -x "$WRKDIR/application.sh" ]; then
	source $WRKDIR/application.sh
fi

cd ..

echo "Building $RELEASENAME.tgz from $RELEASENAME/"
tar zcf $RELEASENAME.tgz $RELEASENAME/

echo "Building $RELEASENAME.tbz2 from $RELEASENAME/"
tar jcf $RELEASENAME.tbz2 $RELEASENAME/

PUBLISH="yes"

ok="0"
if [ "x$AUTOMATIC" != "xyes" ]; then
	echo
	echo "Would you like to publish these releases now?"
	while [ $ok -eq 0 ]; do
		echo -n "[$PUBLISH] "

		read INPUT
		case $INPUT in
			[Yy]*)
				PUBLISH="yes"
				ok=1
				;;
			[Nn]*)
				PUBLISH="no"
				ok=1
				;;
		esac
	done
fi

if [ "x$PUBLISH" = "xyes" ]; then
	scp $RELEASENAME.tgz distfiles-master.atheme.org:/srv/distfiles
	scp $RELEASENAME.tbz2 distfiles-master.atheme.org:/srv/distfiles

	echo
	echo "The releases have been published, and will be available to the entire"
	echo "distribution network within 15 minutes."
fi

echo
echo "Done. If you have any bugs to report, report them against"
echo "the distfiles.atheme.org component at http://bugzilla.atheme.org"
echo "Thanks!"
echo
