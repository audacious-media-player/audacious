#!/bin/sh

# Quick-and-dirty script for updating a Windows release folder

rm -rf /C/aud-win32/share/locale

cd /C/aud-win32
for i in `find -type f` ; do
    if test -f /C/audacious/win32/override/$i ; then
        cp /C/audacious/win32/override/$i $i
    elif test -f /C/msys32/mingw32/$i ; then
        cp /C/msys32/mingw32/$i $i
    elif test -f /C/GTK/$i ; then
        cp /C/GTK/$i $i
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

cd /C/GTK
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
