#!/usr/bin/env bash

# --- Dependency configuration ---
#
# ubuntu-20.04:      Qt 5 + GTK2
# ubuntu-22.04:      Qt 5 + GTK3
# Windows:           Qt 6 + GTK2
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

    # Install Python setuptools for the distutils module which is still
    # required by gdbus-codegen and no longer shipped with Python >= 3.12.
    #
    # - https://peps.python.org/pep-0632
    # - https://github.com/python/cpython/issues/95299
    python3 -m pip install setuptools
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
