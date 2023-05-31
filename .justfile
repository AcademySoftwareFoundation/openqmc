# list available recipes
@default:
  just --list --unsorted

# setup build config, defaults to Ninja
[unix]
@setup generator='Ninja':
  cmake --preset unix -G '{{generator}}'

# set build option to value
[unix]
@set option value:
  cmake --preset unix -D {{option}}={{value}}

# configure all build options via TUI
[unix]
@config:
  ccmake --preset unix

# build target, defaults to all tools
[unix]
@build target='all':
  cmake --build --preset unix --target {{target}}

# build and run all tests
[unix]
@test: (build 'tests')
  ctest --preset unix

# remove build and reset
@clean:
  rm -rf build
