plugins_ldflags += ['-Wl,--undefined=dummy_plugin_register_types']

plugins_sources += files(
  'dummy-action-1.c',
  'dummy-action-factory.c',
  'dummy-plugin.c',
)

plugins_sources += gnome.compile_resources(
  'dummy-resources',
  'dummy.gresource.xml',
  c_name: 'dummy_plugin',
)
