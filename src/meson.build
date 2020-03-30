src_inc = include_directories('.')


config_h = configure_file(input: 'config.h.meson',
  output: 'config.h',
  configuration: conf)


config_h_full_path = join_paths(meson.current_build_dir(), 'config.h')


if cc.get_id() == 'gcc' or cc.get_id() == 'clang'
  add_project_arguments('-include', config_h_full_path, language: 'c')
  add_project_arguments('-include', config_h_full_path, language: 'cpp')
else
  error('Please implement -include handling for your chosen compiler.')
endif


subdir('libguess')
subdir('libaudcore')
subdir('libaudtag')


if get_option('qt')
  subdir('libaudqt')
endif


if get_option('dbus')
  subdir('dbus')
  subdir('audtool')
endif


subdir('audacious')
