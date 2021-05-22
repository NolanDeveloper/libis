project('libis', 'c',
  version: '0.1',
  default_options: ['warning_level=3', 'werror=true', 'c_std=c11'])

include_directories = include_directories('include')

install_headers('include/libis.h')

src = [
  'src/libis.c',
  'src/libis_file_source.c',
  'src/libis_buffer_source.c',
]

if host_machine.system() == 'linux'
  src += 'src/libis_file_descriptor_source.c'
endif

libis = library('is', src, install: true, include_directories: include_directories)

pkg = import('pkgconfig')
pkg.generate(libis)

libis_dep = declare_dependency(
  include_directories: include_directories,
  link_with: libis
)

libis_test_unit = executable(
  'libis_test_unit',
  ['test-unit/main.c'],
  link_with: libis,
  include_directories: include_directories
)

test('unit test', libis_test_unit)
