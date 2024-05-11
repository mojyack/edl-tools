project('edl', 'cpp', version: '1.0.0', default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++20'])

subdir('src/xml')

client_src = files(
  'src/edl-client.cpp',
  'src/firehose-actions.cpp',
  'src/sahara-actions.cpp',
  'src/sahara-packet-stringnize.cpp',
  'src/serial-device.cpp',
) + tinyxml_src

buse_src = files(
  'src/buse/buse.cpp',
  'src/edl-buse.cpp',
  'src/firehose-actions.cpp',
  'src/sahara-packet-stringnize.cpp',
  'src/serial-device.cpp',
  'src/xml/deparser.cpp',
  'src/xml/parser.cpp',
  'src/xml/xml.cpp',
)

executable('client', client_src)
executable('buse', buse_src)
