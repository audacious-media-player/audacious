libguess_sources = [
  'dfa.c',
  'guess.c',
  'guess_impl.c'
]

if cc.has_argument('-Wno-unused-variable')
  add_project_arguments('-Wno-unused-variable', language: 'c')
endif

libguess_lib = static_library('guess',
  libguess_sources,
  c_args: ['-DLIBGUESS_CORE'],
  pic: true
)


libguess_inc = include_directories('.')
