plugins_incs = [
  include_directories('..'),
]

plugins_sources = []
plugins_deps = [ boatswain_deps ]
plugins_ldflags = []
plugins_libs = []
plugins_confs = []

subdir('default')
subdir('obs-studio')
subdir('soundboard')

plugins_lib = static_library(
  'boatswain-plugins',
  plugins_sources,
  dependencies: plugins_deps,
  include_directories: plugins_incs,
  link_with: plugins_libs,
  link_args: plugins_ldflags,
)

plugins_dep = declare_dependency(
  link_whole: plugins_lib,
)
