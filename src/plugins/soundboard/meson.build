plugins_ldflags += ['-Wl,--undefined=soundboard_plugin_register_types']

plugin_file = i18n.merge_file(
  input: 'soundboard.plugin.desktop.in',
  output: 'soundboard.plugin',
  type: 'desktop',
  po_dir: meson.project_source_root() / 'po',
)

# Copied from gnome-settings-daemon
gnome_settings_daemon_sources = files(
  'bus-watch-namespace.c',
  'mpris-controller.c',
)

plugins_sources += gnome_settings_daemon_sources + files(
  'soundboard-action-factory.c',
  'soundboard-mpris-action.c',
  'soundboard-play-action.c',
  'soundboard-play-action-prefs.c',
  'soundboard-plugin.c',
)

plugins_sources += gnome.compile_resources(
  'soundboard-resources',
  'soundboard.gresource.xml',
  c_name: 'soundboard_plugin',
  dependencies: [ plugin_file ],
)
