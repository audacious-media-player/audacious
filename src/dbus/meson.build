gio_req = '>= 2.32'
gio_dep = dependency('gio-2.0', version: gio_req, required: true)
aud_dbus_deps = [gio_dep]


if not have_windows
  gio_unix_dep = dependency('gio-unix-2.0', version: gio_req, required: true)
  aud_dbus_deps += [gio_unix_dep]
endif


aud_dbus_src = gnome.gdbus_codegen('aud-dbus',
  sources: 'aud-dbus.xml',
  interface_prefix: 'org.atheme.',
  namespace: 'Obj'
)
aud_dbus_deps += declare_dependency(sources: aud_dbus_src)


aud_dbus_lib = static_library('aud-dbus',
  aud_dbus_src,
  dependencies: aud_dbus_deps,
  pic: true
)


aud_dbus_inc = include_directories('.')
