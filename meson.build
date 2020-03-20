project('audacious', 'c', 'cpp',
        version: '4.0-devel',
        meson_version: '>= 0.50')


copyright = 'Copyright (C) 2001-2020 Audacious developers and others'


qt5 = import('qt5')
gnome = import('gnome')


glib_req = '>= 2.32'
glib_dep = dependency('glib-2.0', version: glib_req, required: true)
gmodule_dep = dependency('gmodule-2.0', version: glib_req, required: true)


if get_option('qt')
  qt_req = '>= 5.2'
  qt_dep = dependency('qt5', version: qt_req, required: true, modules: ['Core', 'Widgets', 'Gui'])
endif


if get_option('libarchive')
  libarchive_dep = dependency('libarchive', required: true)
endif


cc = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')


if cc.get_id() == 'gcc' or cc.get_id() == 'clang'
  common_flags = [
    '-pipe',
    '-ffast-math',
    '-Wall',
    '-Wtype-limits',
    '-Wno-stringop-truncation'
  ]

  cxx_flags = [
    '-Wno-non-virtual-dtor',
    '-Woverloaded-virtual'
  ]

  check_cflags = ['-std=gnu99'] + common_flags
  check_cxxflags = ['-std=gnu++11'] + common_flags + cxx_flags

  foreach arg : check_cflags
    if cc.has_argument(arg)
      add_project_arguments(arg, language: 'c')
    endif
  endforeach

  foreach arg : check_cxxflags
    if cxx.has_argument(arg)
      add_project_arguments(arg, language: 'cpp')
    endif
  endforeach
endif


conf = configuration_data()
conf.set_quoted('BUILDSTAMP', '???')
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
conf.set_quoted('PLUGIN_SUFFIX', '.so')
if host_machine.system() == 'windows'
  conf.set_quoted('PLUGIN_SUFFIX', '.dll')
elif host_machine.system() == 'darwin'
  conf.set_quoted('PLUGIN_SUFFIX', '.dylib')
endif


if host_machine.system() == 'windows'
  conf.set('EXPORT', '__declspec(dllexport)')
  conf.set_quoted('PLUGIN_SUFFIX', '.dll')
elif cc.get_id() == 'gcc' or cc.get_id() == 'clang'
  add_project_arguments('-fvisibility=hidden', language: 'c')
  add_project_arguments('-fvisibility=hidden', language: 'cpp')
  conf.set('EXPORT', '__attribute__((visibility("default")))')
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


if get_option('libarchive')
  conf.set10('USE_LIBARCHIVE', true)
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
  libraries: ['-L${libdir} -laudcore'],
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
