#!/bin/sh
SONGTITLE=$(audtool current-song)
SONGELAPSED=$(audtool current-song-output-length)
SONGLEN=$(audtool current-song-length)
MIME=$(file -ib "`audtool current-song-filename`")

echo "np: $SONGTITLE [$MIME] ($SONGELAPSED/$SONGLEN)"
