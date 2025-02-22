#!/usr/bin/env bash

# --- Dependency configuration ---
#
# ubuntu-22.04:      Qt 5 + GTK 2
# ubuntu-24.04:      Qt 6 + GTK 3
# Windows:           Qt 6 + GTK 2
# macOS 13:          Qt 5 - GTK
# macOS 15:          Qt 6 - GTK

os=$(tr '[:upper:]' '[:lower:]' <<< "$1")
build_system=$(tr '[:upper:]' '[:lower:]' <<< "$2")

if [ -z "$os" ] || [ -z "$build_system" ]; then
  echo 'Invalid or missing input parameters'
  exit 1
fi

case "$os" in
  ubuntu-22.04)
    if [ "$build_system" = 'meson' ]; then
      sudo apt-get -qq update && sudo apt-get install libgtk2.0-dev qtbase5-dev libqt5svg5-dev meson
    else
      sudo apt-get -qq update && sudo apt-get install libgtk2.0-dev qtbase5-dev libqt5svg5-dev
    fi
    ;;

  ubuntu*)
    if [ "$build_system" = 'meson' ]; then
      sudo apt-get -qq update && sudo apt-get install libgtk-3-dev qt6-base-dev qt6-svg-dev gettext meson
    else
      sudo apt-get -qq update && sudo apt-get install libgtk-3-dev qt6-base-dev qt6-svg-dev gettext
    fi
    ;;

  macos-13)
    if [ "$build_system" = 'meson' ]; then
      brew install qt@5 meson
    else
      brew install qt@5 automake
    fi
    ;;

  macos*)
    if [ "$build_system" = 'meson' ]; then
      brew install qt@6 meson
    else
      brew install qt@6 automake libiconv
    fi
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
