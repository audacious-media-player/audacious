libguess_sources = [
  'dfa.c',
  'guess.c',
  'guess_impl.c'
]


libguess_lib = static_library('guess',
  libguess_sources,
  c_args: ['-DLIBGUESS_CORE'],
  pic: true
)


libguess_inc = include_directories('.')
