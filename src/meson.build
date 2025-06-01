src_inc = include_directories('.')


subdir('libguess')
subdir('libaudcore')
subdir('libaudtag')


if get_option('qt')
  subdir('libaudqt')
endif


if get_option('gtk')
  subdir('libaudgui')
endif


if get_option('dbus')
  subdir('dbus')
  subdir('audtool')
endif


subdir('audacious')
