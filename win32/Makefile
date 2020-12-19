LIB_PREFIX = /C/libs

gtk+-2.24.31.tar.xz:
	wget https://ftp.gnome.org/pub/GNOME/sources/gtk+/2.24/gtk+-2.24.31.tar.xz

gtk: gtk+-2.24.31.tar.xz
	tar xf gtk+-2.24.31.tar.xz
	cd gtk+-2.24.31 && patch gdk/win32/gdkwindow-win32.c ../patches/gdkwindow-win32.c.diff
	cd gtk+-2.24.31 && patch gtk/gtkicontheme.c ../patches/gtkicontheme.c.diff
	cd gtk+-2.24.31 && patch gtk/gtkmain.c ../patches/gtkmain.c.diff
	cd gtk+-2.24.31 && patch -p1 < ../patches/gtk-parentheses.diff
	cd gtk+-2.24.31 && CFLAGS="-O2 -fcommon" ./configure --prefix=$(LIB_PREFIX)
	cd gtk+-2.24.31 && $(MAKE) && $(MAKE) install
