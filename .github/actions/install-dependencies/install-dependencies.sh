#!/usr/bin/env bash

# --- Dependency configuration ---
#
# ubuntu-20.04:      Qt 5 + GTK2
# ubuntu-22.04:      Qt 5 + GTK3
# macOS (Autotools): Qt 5 - GTK
# macOS (Meson):     Qt 6 - GTK

os=$(tr '[:upper:]' '[:lower:]' <<< "$1")
build_system=$(tr '[:upper:]' '[:lower:]' <<< "$2")

if [ -z "$os" ] || [ -z "$build_system" ]; then
  echo 'Invalid or missing input parameters'
  exit 1
fi

case "$os" in
  ubuntu-20.04)
    if [ "$build_system" = 'meson' ]; then
      sudo apt-get -qq update && sudo apt-get install libgtk2.0-dev qtbase5-dev meson
    else
      sudo apt-get -qq update && sudo apt-get install libgtk2.0-dev qtbase5-dev
    fi
    ;;

  ubuntu*)
    if [ "$build_system" = 'meson' ]; then
      sudo apt-get -qq update && sudo apt-get install libgtk-3-dev qtbase5-dev gettext meson
    else
      sudo apt-get -qq update && sudo apt-get install libgtk-3-dev qtbase5-dev gettext
    fi
    ;;

  macos*)
    if [ "$build_system" = 'meson' ]; then
      brew install qt@6 meson
    else
      brew install qt@5 automake
    fi
    ;;

  *)
    echo "Unsupported OS: $os"
    exit 1
    ;;
esac
