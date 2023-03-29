boatswain_sources = [
  'bs-action.c',
  'bs-action-factory.c',
  'bs-application.c',
  'bs-device-manager.c',
  'bs-empty-action.c',
  'bs-icon.c',
  'bs-icon-renderer.c',
  'bs-log.c',
  'bs-omni-bar.c',
  'bs-page.c',
  'bs-page-item.c',
  'bs-profile.c',
  'bs-profile-row.c',
  'bs-profiles-dialog.c',
  'bs-stream-deck.c',
  'bs-stream-deck-button.c',
  'bs-stream-deck-button-editor.c',
  'bs-stream-deck-button-widget.c',
  'bs-stream-deck-editor.c',
  'bs-window.c',
  'main.c',
]

boatswain_deps = [
  dependency('libadwaita-1', version: '>= 1.2'),
  dependency('libpeas-2'),
  dependency('libportal-gtk4'),
  dependency('gusb', version: '>= 0.3.0'),
  dependency('gtk4'),
  dependency('hidapi-libusb'),
  dependency('json-glib-1.0'),
]

subdir('plugins')

gnome = import('gnome')

debug_conf = configuration_data()
debug_conf.set10('ENABLE_TRACING', get_option('tracing'))
debug_conf.set('BUGREPORT_URL', 'https://gitlab.gnome.org/World/boatswain/-/issues/new')

boatswain_sources += configure_file(
  input: 'bs-debug.h.in',
  output: 'bs-debug.h',
  configuration: debug_conf,
)

boatswain_sources += gnome.compile_resources(
  'boatswain-resources',
  'boatswain.gresource.xml',
  c_name: 'boatswain'
)

boatswain_sources += gnome.mkenums(
  'boatswain-enum-types',
  sources: [ 'bs-page-item.h' ],
  h_template: 'bs-enum-types.h.in',
  c_template: 'bs-enum-types.c.in',
)

executable(
  'boatswain',
  boatswain_sources,
  dependencies: [boatswain_deps, plugins_dep],
  install: true,
)
