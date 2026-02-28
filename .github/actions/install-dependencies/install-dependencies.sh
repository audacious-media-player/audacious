#!/usr/bin/env bash

# --- Dependency configuration ---
#
# ubuntu-22.04:      Qt 5 + GTK 2
# ubuntu-24.04:      Qt 6 + GTK 3
# Windows:           Qt 6 + GTK 2
# macOS 15:          Qt 5 - GTK
# macOS 26:          Qt 6 - GTK

os=$(tr '[:upper:]' '[:lower:]' <<< "$1")

if [ -z "$os" ]; then
  echo 'Invalid or missing input parameters'
  exit 1
fi

case "$os" in
  ubuntu-22.04)
    sudo apt-get -qq update && sudo apt-get install libgtk2.0-dev qtbase5-dev libqt5svg5-dev meson
    ;;

  ubuntu*)
    sudo apt-get -qq update && sudo apt-get install libgtk-3-dev qt6-base-dev qt6-svg-dev gettext meson
    ;;

  macos-15*)
    brew update -q && brew install qt@5 meson
    ;;

  macos*)
    brew update -q && brew install qt@6 meson
    ;;

  windows*)
    # - Nothing to do here -
    # Packages are installed with the MSYS2 setup in the action.yml file
    # and by making use of 'paccache'.
    ;;

  *)
    echo "Unsupported OS: $os"
    exit 1
    ;;
esac
