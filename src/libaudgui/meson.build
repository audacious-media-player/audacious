libaudgui_sources = [
  'about.cc',
  'confirm.cc',
  'eq-preset.cc',
  'equalizer.cc',
  'file-opener.cc',
  'gtk-compat.cc',
  'infopopup.cc',
  'infowin.cc',
  'init.cc',
  'jump-to-time.cc',
  'jump-to-track.cc',
  'jump-to-track-cache.cc',
  'list.cc',
  'menu.cc',
  'pixbufs.cc',
  'playlists.cc',
  'plugin-menu.cc',
  'plugin-prefs.cc',
  'plugin-view.cc',
  'prefs-widget.cc',
  'prefs-window.cc',
  'preset-browser.cc',
  'queue-manager.cc',
  'scaled-image.cc',
  'status.cc',
  'urilist.cc',
  'url-opener.cc',
  'util.cc'
]


libaudgui_headers = [
  'gtk-compat.h',
  'libaudgui.h',
  'libaudgui-gtk.h',
  'list.h',
  'menu.h'
]


image_resources = gnome.compile_resources(
  'images', 'images.gresource.xml',
  source_dir: '../../images'
)


install_headers(libaudgui_headers, subdir: 'libaudgui')


libaudgui_lib = library('audgui',
  libaudgui_sources,
  image_resources,
  include_directories: [src_inc],
  dependencies: [gtk_dep, intl_dep],
  link_with: [libaudcore_lib],
  soversion: '6',
  version: '6.0.0',
  install: true
)
