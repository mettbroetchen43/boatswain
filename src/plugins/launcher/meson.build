plugins_ldflags += ['-Wl,--undefined=launcher_plugin_register_types']

plugin_file = i18n.merge_file(
  input: 'launcher.plugin.in',
  output: 'launcher.plugin',
  type: 'desktop',
  po_dir: meson.project_source_root() / 'po',
)

plugins_sources += files(
  'launcher-action-factory.c',
  'launcher-launch-action.c',
  'launcher-launch-preferences.c',
  'launcher-plugin.c',
)

plugins_sources += gnome.compile_resources(
  'launcher-resources',
  'launcher.gresource.xml',
  c_name: 'launcher_plugin',
  dependencies: [ plugin_file ],
)
