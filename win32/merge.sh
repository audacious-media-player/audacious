#!/bin/sh

# Quick-and-dirty script for updating a Windows release folder

srcdir=$(dirname "$(readlink -f "$0")")

rm -rf /C/aud-win32/bin/share
rm -rf /C/aud-win32/share/locale

cd /C/aud-win32
for i in `find -type f` ; do
    if test -f ${srcdir}/override/$i ; then
        cp ${srcdir}/override/$i $i
    elif test -f /C/msys64/mingw64/$i ; then
        cp /C/msys64/mingw64/$i $i
    elif test -f /C/msys64/mingw64/share/qt6/plugins/${i#"./bin/"} ; then
        cp /C/msys64/mingw64/share/qt6/plugins/${i#"./bin/"} $i
    elif test -f /C/libs/$i ; then
        cp /C/libs/$i $i
    elif test -f /C/aud/$i ; then
        cp /C/aud/$i $i
    else
        echo Not found: $i
    fi
done

for i in `find -name *.dll` ; do strip -s $i ; done
for i in `find -name *.exe` ; do strip -s $i ; done

cd /C/msys64/mingw64/share/qt6/translations
mkdir -p /C/aud-win32/bin/share/qt6/translations
for i in `find . -name '*qt_*' ! -name '*qt_help_*' -o -name '*qtbase_*'` ; do
    cp $i /C/aud-win32/bin/share/qt6/translations/$i
done

cd /C/libs
for i in `find ./share/locale -name gtk20.mo` ; do
    mkdir -p /C/aud-win32/${i%%/gtk20.mo}
    cp $i /C/aud-win32/$i
done

cd /C/aud
for i in `find ./share/locale -name audacious.mo` ; do
    mkdir -p /C/aud-win32/${i%%/audacious.mo}
    cp $i /C/aud-win32/$i
done
for i in `find ./share/locale -name audacious-plugins.mo` ; do
    mkdir -p /C/aud-win32/${i%%/audacious-plugins.mo}
    cp $i /C/aud-win32/$i
done
