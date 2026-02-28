#!/usr/bin/env bash

# --- Build configuration ---
#
# ubuntu-22.04:      Qt 5 + GTK 2
# ubuntu-24.04:      Qt 6 + GTK 3
# Windows:           Qt 6 + GTK 2
# macOS 15:          Qt 5 - GTK
# macOS 26:          Qt 6 - GTK

action=$(tr '[:upper:]' '[:lower:]' <<< "$1")
os=$(tr '[:upper:]' '[:lower:]' <<< "$2")

if [ -z "$action" ] || [ -z "$os" ]; then
  echo 'Invalid or missing input parameters'
  exit 1
fi

if [[ "$os" != windows* ]]; then
  _sudo='sudo'
fi

if [ -d 'audacious' ]; then
  cd audacious
fi

case "$action" in
  configure)
    case "$os" in
      ubuntu-22.04)
        meson setup build -D qt5=true -D gtk2=true
        ;;

      ubuntu*)
        meson setup build
        ;;

      macos-15*)
        export PATH="/usr/local/opt/qt@5/bin:$PATH"
        export PKG_CONFIG_PATH="/usr/local/opt/qt@5/lib/pkgconfig:$PKG_CONFIG_PATH"
        meson setup build -D qt5=true -D gtk=false
        ;;

      macos*)
        export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"
        export PKG_CONFIG_PATH="/opt/homebrew/opt/qt@6/libexec/lib/pkgconfig:$PKG_CONFIG_PATH"
        meson setup build -D gtk=false
        ;;

      windows*)
        meson setup build -D gtk2=true
        ;;

      *)
        echo "Unsupported OS: $os"
        exit 1
        ;;
    esac
    ;;

  build)
    meson compile -C build
    ;;

  test)
    cd src/libaudcore/tests
    meson setup build && meson test -v -C build
    ;;

  install)
    $_sudo meson install -C build
    ;;

  *)
    echo "Unsupported action: $action"
    exit 1
    ;;
esac
