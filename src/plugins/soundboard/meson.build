plugins_ldflags += ['-Wl,--undefined=soundboard_plugin_register_types']

plugins_sources += files(
  'soundboard-play-action.c',
  'soundboard-play-action-prefs.c',
  'soundboard-action-factory.c',
  'soundboard-plugin.c',
)

plugins_sources += gnome.compile_resources(
  'soundboard-resources',
  'soundboard.gresource.xml',
  c_name: 'soundboard_plugin',
)
