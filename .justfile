# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

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
  ccmake --preset base

# build target, defaults to all tools
@build target='all':
  cmake --build --preset {{os_family()}} --target {{target}}

# build and run all tests
@test: (build 'tests')
  ctest --preset {{os_family()}}

# open up a jupyter notebook
@notebook: build
  jupyter notebook --ip 0.0.0.0 --allow-root python/notebooks

# run notebook and save readme images
@readme-images: build
  SAVEPLOT=light jupyter execute python/notebooks/readme-images.ipynb
  SAVEPLOT=dark jupyter execute python/notebooks/readme-images.ipynb

# run notebook and save performance images
@performance-images: build
  SAVEPLOT=light jupyter execute python/notebooks/performance.ipynb
  SAVEPLOT=dark jupyter execute python/notebooks/performance.ipynb

# build docs
@docs:
  doxygen

# remove build and reset
@clean:
  rm -rf build html result
