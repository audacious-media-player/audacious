i18n = import('i18n')


i18n.gettext('audacious',
  args: [
    '--default-domain=audacious',
    '--language=C',
    '--keyword=_',
    '--keyword=N_',
    '--from-code=UTF-8',
    '--msgid-bugs-address="https://redmine.audacious-media-player.org/"',
    '--directory=%0%'.format(meson.source_root()),
    '--files-from=POTFILES.in'
  ]
)
