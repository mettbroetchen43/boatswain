plugins_ldflags += ['-Wl,--undefined=network_plugin_register_types']

plugin_file = i18n.merge_file(
  input: 'network.plugin.desktop.in',
  output: 'network.plugin',
  type: 'desktop',
  po_dir: meson.project_source_root() / 'po',
)

plugins_sources += files(
  'network-action-factory.c',
  'network-http-action.c',
  'network-http-action-prefs.c',
  'network-plugin.c',
)

plugins_deps += [
  dependency('libsoup-3.0'),
]

plugins_sources += gnome.compile_resources(
  'network-resources',
  'network.gresource.xml',
  c_name: 'network_plugin',
  dependencies: [ plugin_file ],
)
