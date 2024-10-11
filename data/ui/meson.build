#
# Blueprints
#

blp = files(
  'window.blp'
)

ui = custom_target(
  'ui',
  input: blp,
  output: '.',
  command: [
    find_program('blueprint-compiler'), 'batch-compile', '@OUTPUT@', '@CURRENT_SOURCE_DIR@', '@INPUT@'
  ]
)
