audtool_sources = [
  'main.c',
  'handlers_general.c',
  'handlers_playback.c',
  'handlers_playlist.c',
  'handlers_playqueue.c',
  'handlers_vitals.c',
  'handlers_equalizer.c',
  'report.c',
  'wrappers.c'
]


audtool_deps = aud_dbus_deps


audtool_exe = executable('audtool',
  audtool_sources,
  link_with: aud_dbus_lib,
  dependencies: audtool_deps,
  include_directories: aud_dbus_inc,
  install: true
)
