libaudqt_sources = [
  'about-qt.cc',
  'art-qt.cc',
  'audqt.cc',
  'colorbutton.cc',
  'eq-preset-qt.cc',
  'equalizer-qt.cc',
  'file-entry.cc',
  'fileopener.cc',
  'font-entry.cc',
  'infopopup-qt.cc',
  'infowin-qt.cc',
  'info-widget.cc',
  'log-inspector.cc',
  'menu-qt.cc',
  'playlist-management.cc',
  'plugin-menu-qt.cc',
  'prefs-builder.cc',
  'prefs-plugin.cc',
  'prefs-widget-qt.cc',
  'prefs-window-qt.cc',
  'prefs-pluginlist-model.cc',
  'queue-manager-qt.cc',
  'url-opener-qt.cc',
  'util-qt.cc',
  'treeview.cc',
  'volumebutton.cc'
]


libaudqt_headers = [
  'colorbutton.h',
  'export.h',
  'iface.h',
  'info-widget.h',
  'libaudqt.h',
  'menu.h',
  'treeview.h'
]


images_src = qt5.preprocess(qresources: 'images.qrc')


install_headers(libaudqt_headers, subdir: 'libaudqt')


libaudqt_lib = library('audqt',
  libaudqt_sources,
  images_src,
  cpp_args: ['-DLIBAUDQT_BUILD'],
  include_directories: [src_inc],
  dependencies: [qt_dep],
  link_with: [libaudcore_lib],
  soversion: '2',
  version: '2.2.0',
  install: true
)
