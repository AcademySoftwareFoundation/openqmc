# list available recipes
@default:
  just --list --unsorted

# setup build config, defaults to Ninja
@setup generator='Ninja':
  cmake --preset {{os_family()}} -G '{{generator}}'

# set build option to value
@set option value:
  cmake --preset {{os_family()}} -D {{option}}={{value}}

# configure all build options via TUI
@config:
  ccmake --preset {{os_family()}}

# build target, defaults to all tools
@build target='all':
  cmake --build --preset {{os_family()}} --target {{target}}

# build and run all tests
@test: (build 'tests')
  ctest --preset {{os_family()}}

# open up a jupyter notebook
@notebook: build
  jupyter notebook python/notebooks

# remove build and reset
@clean:
  rm -rf build
