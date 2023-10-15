plugin_file = i18n.merge_file(
  input: 'gaming.plugin.desktop.in',
  output: 'gaming.plugin',
  type: 'desktop',
  po_dir: meson.project_source_root() / 'po',
)

plugins_sources += gnome.compile_resources(
  'gaming-resources',
  'gaming.gresource.xml',
  c_name: 'gaming_plugin',
  dependencies: [ plugin_file ],
)
