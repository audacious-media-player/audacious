#!/bin/sh
UNUSED=0
while [  $UNUSED -lt 1 ]; do
	PLAYTIME=$(audtool current-song-output-length)
	SONGTITLE=$(audtool current-song)
	SONGLEN=$(audtool current-song-length)
	echo -n "[>] $PLAYTIME $SONGTITLE ($SONGLEN)                     "
	sleep 1
	printf "\r"
done
