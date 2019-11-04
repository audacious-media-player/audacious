mans = ['audtool.1', 'audacious.1']

foreach man : mans
  configure_file(input: '@0@.in'.format(man),
    output: man,
    configuration: conf,
    install: true,
    install_dir: join_paths(get_option('mandir'), 'man1'))
endforeach
