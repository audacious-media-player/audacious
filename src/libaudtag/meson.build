libaudtag_sources = [
  'audtag.cc',
  'util.cc',
  'tag_module.cc',
  'id3/id3-common.cc',
  'id3/id3v1.cc',
  'id3/id3v22.cc',
  'id3/id3v24.cc',
  'ape/ape.cc'
]


libaudtag_inc = include_directories('.')


install_headers('audtag.h', subdir: 'audacious')


libaudtag_lib = library('audtag',
  libaudtag_sources,
  include_directories: src_inc,
  dependencies: [glib_dep],
  link_with: libaudcore_lib,
  version: '3.0.0',
  soversion: 3,
  install: true
)
