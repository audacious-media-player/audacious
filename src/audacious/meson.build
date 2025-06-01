audacious_deps = [glib_dep, thread_dep]
audacious_libs = [libaudcore_lib]
audacious_incs = [src_inc]

audacious_sources = [
  'main.cc',
  'signals.cc',
  'util.cc'
]


if get_option('dbus')
  audacious_sources += ['dbus-server.cc']
  audacious_libs += [aud_dbus_lib]
  audacious_deps += [aud_dbus_deps]
  audacious_incs += [aud_dbus_inc]
endif


if have_windows
  windows = import('windows')
  audacious_sources += windows.compile_resources('audacious.rc',
    include_directories: meson.current_build_dir()
  )
endif


audacious_exe = executable('audacious',
  audacious_sources,
  include_directories: audacious_incs,
  dependencies: audacious_deps,
  link_with: audacious_libs,
  install: true
)
