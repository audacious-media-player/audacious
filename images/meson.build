foreach size : [16, 24, 32, 48, 64, 96, 128, 256]
  install_data('audacious-@0@.png'.format(size),
    install_dir: join_paths(install_iconpath, 'hicolor', '@0@x@0@'.format(size), 'apps'),
    rename: 'audacious.png'
  )
endforeach

install_data('audacious.svg', install_dir: join_paths(install_iconpath, 'hicolor', 'scalable', 'apps'))
