project('libaudcore-tests', 'cpp',
        version: '0.1.0',
        meson_version: '>= 0.43',
        default_options: [
          'cpp_std=c++11',
          'warning_level=1'
        ])

glib_dep = dependency('glib-2.0', version: '>= 2.32')
qt_dep = dependency('qt5', version: '>= 5.2', modules: ['Core'])
thread_dep = dependency('threads')


test_sources = [
  '../audio.cc',
  '../audstrings.cc',
  '../charset.cc',
  '../hook.cc',
  '../index.cc',
  '../logger.cc',
  '../mainloop.cc',
  '../multihash.cc',
  '../ringbuf.cc',
  '../stringbuf.cc',
  '../strpool.cc',
  '../tinylock.cc',
  '../threads.cc',
  '../tuple.cc',
  '../tuple-compiler.cc',
  '../util.cc',
  'stubs.cc',
  'test.cc',
  'test-mainloop.cc'
]


cxx = meson.get_compiler('cpp')

add_project_arguments(cxx.get_supported_arguments([
  '-fno-elide-constructors',
  '-fprofile-arcs',
  '-ftest-coverage'
]), language: 'cpp')


add_project_arguments([
  '-DEXPORT=',
  '-DPACKAGE="audacious"',
  '-DICONV_CONST=',
  '-DUSE_QT'
], language: 'cpp')


conf = configuration_data()
conf.set10('BIGENDIAN', host_machine.endian() == 'big')


audio_h = configure_file(
  input: '../audio.h.in',
  output: 'audio.h',
  configuration: conf
)


test_exe = executable('libaudcore-tests',
  test_sources,
  include_directories: ['..', '../..'],
  dependencies: [glib_dep, qt_dep, thread_dep],
  link_args: ['-lgcov', '--coverage']
)


test('libaudcore', test_exe)
