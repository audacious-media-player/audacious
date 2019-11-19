audacious_deps = [glib_dep]
audacious_libs = [libaudcore_lib]

audacious_sources = [
  'main.cc',
  'signals.cc',
  'util.cc'
]


if get_option('dbus')
  audacious_sources += ['dbus-server.cc']
  audacious_libs += [aud_dbus_lib]
  audacious_deps += [aud_dbus_deps]
endif


audacious_exe = executable('audacious',
  audacious_sources,
  include_directories: [src_inc, aud_dbus_inc],
  dependencies: audacious_deps,
  link_with: audacious_libs,
  install: true
)
