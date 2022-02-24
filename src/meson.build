boatswain_sources = [
  'bs-action.c',
  'bs-action-factory.c',
  'bs-application.c',
  'bs-device-manager.c',
  'bs-icon.c',
  'bs-icon-renderer.c',
  'bs-stream-deck.c',
  'bs-stream-deck-button.c',
  'bs-stream-deck-button-editor.c',
  'bs-stream-deck-editor.c',
  'bs-window.c',
  'main.c',
]

boatswain_deps = [
  dependency('libadwaita-1'),
  dependency('libpeas-1.0', version: '>= 1.17'),
  dependency('libportal-gtk4'),
  dependency('gusb', version: '>= 0.3.0'),
  dependency('gtk4'),
  dependency('hidapi-libusb'),
]

subdir('plugins')

gnome = import('gnome')

boatswain_sources += gnome.compile_resources(
  'boatswain-resources',
  'boatswain.gresource.xml',
  c_name: 'boatswain'
)

executable(
  'boatswain',
  boatswain_sources,
  dependencies: [boatswain_deps, plugins_dep],
  install: true,
)
