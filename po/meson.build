i18n = import('i18n')


i18n.gettext('audacious',
  args: [
    '--default-domain=audacious',
    '--language=C',
    '--keyword=_',
    '--keyword=N_',
    '--from-code=UTF-8',
    '--msgid-bugs-address="https://github.com/audacious-media-player/audacious/issues"',
    '--directory=%0%'.format(meson.current_source_dir()),
    '--files-from=POTFILES.in'
  ]
)
