#!/bin/sh
#
# This script is used to convert the old $HOME/.audacious config dir
# to the new XDG basedir equivalent.
#

: ${BMP_RCPATH:=".audacious"}
: ${XDG_CONFIG_HOME:="$HOME/.config"}
: ${XDG_DATA_HOME:="$HOME/.local/share"}
: ${XDG_CACHE_HOME:="$HOME/.cache"}

rm -fr "$XDG_CONFIG_HOME/audacious"
rm -fr "$XDG_DATA_HOME/audacious"
rm -fr "$XDG_CACHE_HOME/audacious"

mkdir -p "$XDG_CONFIG_HOME/audacious"
mkdir -p "$XDG_DATA_HOME/audacious/Plugins"
mkdir -p "$XDG_DATA_HOME/audacious/Skins"
mkdir -p "$XDG_CACHE_HOME/audacious/thumbs"

mv "$HOME/$BMP_RCPATH/config"        "$XDG_CONFIG_HOME/audacious/"
mv "$HOME/$BMP_RCPATH/playlist.xspf" "$XDG_CONFIG_HOME/audacious/"
mv "$HOME/$BMP_RCPATH/Plugins"       "$XDG_DATA_HOME/audacious/"
mv "$HOME/$BMP_RCPATH/Skins"         "$XDG_DATA_HOME/audacious/"
mv "$HOME/$BMP_RCPATH/accels"        "$XDG_CONFIG_HOME/audacious/"
mv "$HOME/$BMP_RCPATH/log"           "$XDG_CONFIG_HOME/audacious/"

echo "Conversion done. Please move the remaining files manually."
