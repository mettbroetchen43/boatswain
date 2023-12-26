plugins_ldflags += ['-Wl,--undefined=desktop_plugin_register_types']

plugin_file = i18n.merge_file(
  input: 'desktop.plugin.desktop.in',
  output: 'desktop.plugin',
  type: 'desktop',
  po_dir: meson.project_source_root() / 'po',
)

plugins_sources += files(
  'desktop-action-factory.c',
  'desktop-keyboard-shortcut-action.c',
  'desktop-shortcut-dialog.c',
  'desktop-plugin.c',
)

plugins_sources += gnome.compile_resources(
  'desktop-resources',
  'desktop.gresource.xml',
  c_name: 'desktop_plugin',
  dependencies: [ plugin_file ],
)
