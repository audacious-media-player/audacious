#!/usr/bin/env bash

# --- Build configuration ---
#
# ubuntu-20.04:      Qt 5 + GTK2
# ubuntu-22.04:      Qt 5 + GTK3
# Windows:           Qt 6 + GTK2
# macOS (Autotools): Qt 5 - GTK
# macOS (Meson):     Qt 6 - GTK

action=$(tr '[:upper:]' '[:lower:]' <<< "$1")
os=$(tr '[:upper:]' '[:lower:]' <<< "$2")
build_system=$(tr '[:upper:]' '[:lower:]' <<< "$3")

if [ -z "$action" ] || [ -z "$os" ] || [ -z "$build_system" ]; then
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
      ubuntu-20.04)
        if [ "$build_system" = 'meson' ]; then
          meson setup build -D qt5=true
        else
          ./autogen.sh && ./configure
        fi
        ;;

      ubuntu*)
        if [ "$build_system" = 'meson' ]; then
          meson setup build -D qt5=true -D gtk3=true
        else
          ./autogen.sh && ./configure --enable-gtk3
        fi
        ;;

      macos*)
        if [ "$build_system" = 'meson' ]; then
          meson setup build -D gtk=false
        else
          export PATH="/usr/local/opt/qt@5/bin:$PATH"
          export PKG_CONFIG_PATH="/usr/local/opt/qt@5/lib/pkgconfig:$PKG_CONFIG_PATH"
          ./autogen.sh && ./configure --disable-gtk
        fi
        ;;

      windows*)
        if [ "$build_system" = 'meson' ]; then
          meson setup build
        else
          ./autogen.sh && ./configure
        fi
        ;;

      *)
        echo "Unsupported OS: $os"
        exit 1
        ;;
    esac
    ;;

  build)
    if [ "$build_system" = 'meson' ]; then
      ninja -C build
    elif [[ "$os" == macos* ]]; then
      make -j$(sysctl -n hw.logicalcpu)
    else
      make -j$(nproc)
    fi
    ;;

  test)
    cd src/libaudcore/tests
    if [ "$build_system" = 'meson' ]; then
      meson setup build && meson test -v -C build
    elif [[ "$os" == macos* ]]; then
      make -j$(sysctl -n hw.logicalcpu) test && ./test
    else
      make -j$(nproc) test && ./test
    fi
    ;;

  install)
    if [ "$build_system" = 'meson' ]; then
      $_sudo meson install -C build
    else
      $_sudo make install
    fi
    ;;

  *)
    echo "Unsupported action: $action"
    exit 1
    ;;
esac
