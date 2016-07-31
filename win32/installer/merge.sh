#!/bin/sh

# Quick-and-dirty script for updating a Windows release folder

cd /C/aud-win32
for i in `find -type f` ; do
    if test -f /C/MinGW/$i ; then
        cp /C/MinGW/$i $i
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

cd /C/misc
cp loaders.cache /C/aud-win32/lib/gdk-pixbuf-2.0/2.10.0
cp pango.modules /C/aud-win32/etc/pango

rm -rf /C/aud-win32/share/locale

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
