mans = ['audtool.1', 'audacious.1']

foreach man : mans
  configure_file(input: '@0@.in'.format(man), output: man, configuration: conf)
endforeach

install_man(mans)
