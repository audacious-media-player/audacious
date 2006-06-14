#!/bin/sh
SONGTITLE=$(audtool current-song | sed 's: - : 09-12 :g')
SONGELAPSED=$(audtool current-song-output-length)
SONGLEN=$(audtool current-song-length)
SONGPOS=$(audtool playlist-position)
PLAYLEN=$(audtool playlist-length)

printf "\00303-=[\003 \00312$SONGPOS\00309/\00312$PLAYLEN\00309:\00312 $SONGTITLE \00309(\00312$SONGELAPSED\00309/\00312$SONGLEN\00309)\00303 ]=-\003\n"
