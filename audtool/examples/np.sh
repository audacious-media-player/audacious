#!/bin/sh
SONGTITLE=$(audtool current-song)
SONGELAPSED=$(audtool current-song-output-length)
SONGLEN=$(audtool current-song-length)

echo "np: $SONGTITLE ($SONGELAPSED/$SONGLEN)"
