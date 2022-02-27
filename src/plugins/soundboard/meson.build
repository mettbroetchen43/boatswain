plugins_ldflags += ['-Wl,--undefined=soundboard_plugin_register_types']

plugin_file = i18n.merge_file(
  input: 'soundboard.plugin.in',
  output: 'soundboard.plugin',
  type: 'desktop',
  po_dir: '../../po',
)

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
  dependencies: [ plugin_file ],
)
