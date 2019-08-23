libaudcore_inc = include_directories('.')


libaudcore_sources = [
  'adder.cc',
  'art.cc',
  'art-search.cc',
  'audio.cc',
  'audstrings.cc',
  'charset.cc',
  'config.cc',
  'cue-cache.cc',
  'drct.cc',
  'effect.cc',
  'equalizer.cc',
  'equalizer-preset.cc',
  'eventqueue.cc',
  'fft.cc',
  'history.cc',
  'hook.cc',
  'index.cc',
  'inifile.cc',
  'interface.cc',
  'list.cc',
  'logger.cc',
  'mainloop.cc',
  'multihash.cc',
  'output.cc',
  'parse.cc',
  'playback.cc',
  'playlist.cc',
  'playlist-cache.cc',
  'playlist-data.cc',
  'playlist-files.cc',
  'playlist-utils.cc',
  'plugin-init.cc',
  'plugin-load.cc',
  'plugin-registry.cc',
  'preferences.cc',
  'probe.cc',
  'probe-buffer.cc',
  'ringbuf.cc',
  'runtime.cc',
  'scanner.cc',
  'stringbuf.cc',
  'strpool.cc',
  'tinylock.cc',
  'timer.cc',
  'tuple.cc',
  'tuple-compiler.cc',
  'util.cc',
  'vfs.cc',
  'vfs_async.cc',
  'vfs_local.cc',
  'vis-runner.cc',
  'visualization.cc'
]


libaudcore_headers = [
  'audio.h',
  'audstrings.h',
  'drct.h',
  'equalizer.h',
  'export.h',
  'hook.h',
  'i18n.h',
  'index.h',
  'inifile.h',
  'interface.h',
  'list.h',
  'mainloop.h',
  'multihash.h',
  'objects.h',
  'playlist.h',
  'plugin.h',
  'plugins.h',
  'preferences.h',
  'probe.h',
  'ringbuf.h',
  'runtime.h',
  'templates.h',
  'tinylock.h',
  'threads.h',
  'tuple.h',
  'visualizer.h',
  'vfs.h',
  'vfs_async.h'
]


libaudcore_deps = [glib_dep, gmodule_dep]

if get_option('qt')
  libaudcore_deps += [qt_dep]
endif


libaudcore_lib = library('audcore',
  libaudcore_sources,
  cpp_args: ['-DLIBAUDCORE_BUILD'],
  include_directories: src_inc,
  dependencies: libaudcore_deps,
  link_with: libguess_lib,
  version: '5.1.0',
  soversion: '5',
  install: true
)


install_headers(libaudcore_headers, subdir: 'libaudcore')
