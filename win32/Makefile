LIB_PREFIX = /C/libs

gtk+-2.24.33.tar.xz:
	wget https://ftp.gnome.org/pub/GNOME/sources/gtk+/2.24/gtk+-2.24.33.tar.xz

gtk: gtk+-2.24.33.tar.xz
	tar xf gtk+-2.24.33.tar.xz
	cd gtk+-2.24.33 && patch gdk/win32/gdkwindow-win32.c ../patches/gdkwindow-win32.c.diff
	cd gtk+-2.24.33 && CFLAGS="-O2 -fcommon" ./configure --prefix=$(LIB_PREFIX)
	cd gtk+-2.24.33 && $(MAKE) && $(MAKE) install

ffmpeg-6.1.1.tar.xz:
	wget https://ffmpeg.org/releases/ffmpeg-6.1.1.tar.xz

ffmpeg: ffmpeg-6.1.1.tar.xz
	tar xf ffmpeg-6.1.1.tar.xz
	cd ffmpeg-6.1.1 && ./configure --prefix=$(LIB_PREFIX) --enable-shared --cpu=x86-64 --disable-vulkan --disable-x86asm
	cd ffmpeg-6.1.1 && $(MAKE) && $(MAKE) install

libmms-0.6.4.tar.gz:
	wget https://sourceforge.net/projects/libmms/files/libmms/0.6.4/libmms-0.6.4.tar.gz

libmms: libmms-0.6.4.tar.gz
	tar xf libmms-0.6.4.tar.gz
	cd libmms-0.6.4 && patch -p1 < ../patches/libmms-strndup.diff
	cd libmms-0.6.4 && cp /usr/share/libtool/build-aux/ltmain.sh .
	cd libmms-0.6.4 && autoreconf
	cd libmms-0.6.4 && ./configure --prefix=$(LIB_PREFIX)
	cd libmms-0.6.4 && $(MAKE) && $(MAKE) install

libbinio-1.5.tar.bz2:
	wget https://github.com/adplug/libbinio/releases/download/libbinio-1.5/libbinio-1.5.tar.bz2

libbinio: libbinio-1.5.tar.bz2
	tar xf libbinio-1.5.tar.bz2
	cd libbinio-1.5 && ./configure --prefix=$(LIB_PREFIX)
	cd libbinio-1.5 && $(MAKE) && $(MAKE) install

adplug-2.3.3.tar.bz2:
	wget https://github.com/adplug/adplug/releases/download/adplug-2.3.3/adplug-2.3.3.tar.bz2

adplug: adplug-2.3.3.tar.bz2
	tar xf adplug-2.3.3.tar.bz2
	cd adplug-2.3.3 && ./configure --prefix=$(LIB_PREFIX)
	cd adplug-2.3.3 && $(MAKE) && $(MAKE) install

libsidplayfp-2.6.0.tar.gz:
	wget https://sourceforge.net/projects/sidplay-residfp/files/libsidplayfp/2.6/libsidplayfp-2.6.0.tar.gz

libsidplayfp: libsidplayfp-2.6.0.tar.gz
	tar xf libsidplayfp-2.6.0.tar.gz
	cd libsidplayfp-2.6.0 && ./configure --prefix=$(LIB_PREFIX)
	cd libsidplayfp-2.6.0 && $(MAKE) && $(MAKE) install
