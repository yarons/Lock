#
# Build
#

config_h = configure_file(
  input: 'config.h.in',
  output: 'config.h',
  configuration: conf
)

src = files(
  'main.c',
  'application.c',
  'window.c'
)

internal_inc = include_directories('.')

exe = executable(
          project_exec,
                   src,
   include_directories: [internal_inc],
          dependencies: [gtk_dep, adwaita_dep],
               install: true
)
