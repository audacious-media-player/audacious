project('audacious', 'c', 'cpp',
        version: '4.4.1',
        meson_version: '>= 0.46',
        default_options: [
          'c_std=gnu99',
          'cpp_std=gnu++17',
          'warning_level=1'
        ])


copyright = 'Copyright (C) 2001-2024 Audacious developers and others'


gnome = import('gnome')


glib_req = '>= 2.32'
glib_dep = dependency('glib-2.0', version: glib_req, required: true)
gmodule_dep = dependency('gmodule-2.0', version: glib_req, required: true)
thread_dep = dependency('threads', required: true)


if get_option('qt')
  if get_option('qt5')
    qt = import('qt5')
    qt_req = '>= 5.2'
    qt_dep = dependency('qt5', version: qt_req, required: true, modules: ['Core', 'Widgets', 'Gui', 'Svg'])
  else
    if meson.version().version_compare('>= 0.57')
      qt = import('qt6')
    else
      error('Qt 6 is only supported since Meson 0.57.')
    endif

    qt_req = '>= 6.0'
    qt_dep = dependency('qt6', version: qt_req, required: true, modules: ['Core', 'Widgets', 'Gui', 'Svg'])
  endif
endif


if get_option('gtk')
  if get_option('gtk2')
    gtk_req = '>= 2.24'
    gtk_dep = dependency('gtk+-2.0', version: gtk_req, required: true)
  else
    gtk_req = '>= 3.18'
    gtk_dep = dependency('gtk+-3.0', version: gtk_req, required: true)
  endif
endif


if get_option('libarchive')
  libarchive_dep = dependency('libarchive', required: true)
endif


have_darwin = host_machine.system() == 'darwin'
have_windows = host_machine.system() == 'windows'
have_cygwin = host_machine.system() == 'cygwin'


cc = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')


if cc.get_id() in ['gcc', 'clang']
  common_flags = [
    '-ffast-math',
    '-Wtype-limits',
    '-Wno-stringop-truncation',
    '-fvisibility=hidden'
  ]

  cxx_flags = [
    '-Wno-non-virtual-dtor',
    '-Woverloaded-virtual'
  ]

  check_cflags = common_flags
  check_cxxflags = common_flags + cxx_flags

  add_project_arguments(cc.get_supported_arguments(check_cflags), language: 'c')
  add_project_arguments(cxx.get_supported_arguments(check_cxxflags), language: 'cpp')
endif


conf = configuration_data()
conf.set_quoted('BUILDSTAMP', get_option('buildstamp'))
conf.set_quoted('COPYRIGHT', copyright)
conf.set_quoted('PACKAGE', meson.project_name())
conf.set_quoted('VERSION', meson.project_version())
conf.set('PACKAGE_VERSION', meson.project_version())
if host_machine.endian() == 'big'
  conf.set10('WORDS_BIGENDIAN', true)
  conf.set10('BIGENDIAN', true)
else
  conf.set10('BIGENDIAN', false)
endif


# XXX - investigate to see if we can do better
if have_windows or have_cygwin
  conf.set_quoted('PLUGIN_SUFFIX', '.dll')
elif have_darwin
  conf.set_quoted('PLUGIN_SUFFIX', '.dylib')
else
  conf.set_quoted('PLUGIN_SUFFIX', '.so')
endif


if have_windows
  conf.set('EXPORT', '__declspec(dllexport)')
elif cxx.has_argument('-fvisibility=default')
  conf.set('EXPORT', '__attribute__((visibility("default")))')
else
  error('Could not define EXPORT keyword for public symbols.')
endif


if cxx.has_function('sigwait')
  conf.set10('HAVE_SIGWAIT', true)
endif


if meson.version().version_compare('>= 0.59')
  intl_dep = dependency('intl', required: false)
  intl_found = intl_dep.found()
else
  if cxx.has_function('ngettext', prefix: '#include <libintl.h>')
    intl_dep = dependency('', required: false)
    intl_found = true
  else
    intl_dep = cxx.find_library('intl', required: false)
    intl_found = intl_dep.found()
  endif
endif

if intl_found
  conf.set10('HAVE_GETTEXT', true)
endif


install_bindir = get_option('bindir')
install_datadir = join_paths(get_option('datadir'), 'audacious')
install_plugindir = join_paths(get_option('libdir'), 'audacious')
install_localedir = get_option('localedir')
install_desktoppath = join_paths(get_option('datadir'), 'applications')
install_desktopfile = join_paths(install_desktoppath, 'audacious.desktop')
install_iconpath = join_paths(get_option('datadir'), 'icons')
install_unscalable_iconpath = join_paths(install_iconpath, 'hicolor', '48x48', 'apps')
install_scalable_iconpath = join_paths(install_iconpath, 'hicolor', 'scalable', 'apps')
install_iconfile = join_paths(install_unscalable_iconpath, 'audacious.png')


conf.set_quoted('INSTALL_BINDIR', join_paths(get_option('prefix'), install_bindir))
conf.set_quoted('INSTALL_DATADIR', join_paths(get_option('prefix'), install_datadir))
conf.set_quoted('INSTALL_PLUGINDIR', join_paths(get_option('prefix'), install_plugindir))
conf.set_quoted('INSTALL_LOCALEDIR', join_paths(get_option('prefix'), install_localedir))
conf.set_quoted('INSTALL_DESKTOPFILE', join_paths(get_option('prefix'), install_desktopfile))
conf.set_quoted('INSTALL_ICONFILE', join_paths(get_option('prefix'), install_iconfile))
conf.set('plugindir', install_plugindir)
conf.set('datarootdir', get_option('datadir'))


if get_option('dbus')
  conf.set10('USE_DBUS', true)
endif


if get_option('qt')
  conf.set10('USE_QT', true)
endif


if get_option('gtk')
  conf.set10('USE_GTK', true)
  if not (get_option('gtk2'))
    conf.set10('USE_GTK3', true)
  endif
endif


if get_option('libarchive')
  conf.set10('USE_LIBARCHIVE', true)
endif


if get_option('valgrind')
  conf.set10('VALGRIND_FRIENDLY', true)
endif


subdir('src')
subdir('po')
subdir('man')
subdir('images')


install_data('AUTHORS', 'COPYING')
install_data('audacious.desktop', install_dir: install_desktoppath)


pkg = import('pkgconfig')

# When Meson fixes the utter disappointment that is
# https://github.com/mesonbuild/meson/issues/5836,
# use libaudcore_lib as base dependency here.
pkg.generate(
  libraries: ['-L${libdir} -laudcore', intl_dep],
  variables: [
    'plugin_dir=${libdir}/audacious',

    # Appease broken third-party plugin build systems.
    'audacious_include_dir=${includedir}',
    'include_dir=${includedir}',
    'lib_dir=${libdir}'
  ],
  description: 'versatile and handy multi platform media player',
  name: 'audacious',
  url: 'https://audacious-media-player.org'
)


if meson.version().version_compare('>= 0.53')
  summary({
    'Prefix': get_option('prefix'),
    'Bin dir': get_option('bindir'),
    'Lib dir': get_option('libdir'),
    'Data dir': get_option('datadir'),
  }, section: 'Directories')

  summary({
    'D-Bus support': get_option('dbus'),
    'Qt 5 support': get_option('qt5'),
    'Qt 6 support': get_option('qt') and not get_option('qt5'),
    'GTK 2 support': get_option('gtk2'),
    'GTK 3 support': get_option('gtk') and not get_option('gtk2'),
    'Libarchive support': get_option('libarchive'),
    'Valgrind analysis support': get_option('valgrind'),
    'Build stamp': get_option('buildstamp'),
  }, section: 'Configuration')
endif
