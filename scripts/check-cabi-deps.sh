#!/usr/bin/env bash

# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

# Check that the shared C ABI library depends on nothing beyond the C standard
# library (libc), so that C consumers don't acquire other dependencies, such as
# the C++ runtime. Check the library links only libc and exports only the C ABI
# functions. The LLVM tools read both ELF and Mach-O, so the same commands work
# on Linux and macOS. Also check and warn about unexpected symbol imports.

set -eo pipefail

# Libraries the library may link.
allowed_libraries='^(libc\.so\.6|/usr/lib/libSystem\.B\.dylib)$'

# Symbols the library may import, ignoring the underscore prefix on macOS. The
# aim is to reduce this to nothing.
allowed_symbols='^_?(free|malloc|memcpy)$'

# Number of functions in the C ABI. Update when the API changes.
export_count=12

if llvm-readobj --file-header "$1" | grep -q '^Format: elf'; then
	elf=1
fi

size=$(du -h "$1" | cut -f1)
libraries=$(llvm-readobj --needed-libs "$1" | awk '/^\]/{f=0} f{print $1} /^NeededLibraries/{f=1}' | grep -v -F "$(basename "$1")" || true)
exports=$(llvm-nm ${elf:+--dynamic} -g --defined-only "$1" | awk '$2 == "T" {print $3}')
imports=$(llvm-nm ${elf:+--dynamic} -g --undefined-only "$1" | awk '{print $NF}')

echo "Size of $1:"
echo "${size:-(none)}"
echo
echo "Libraries of $1:"
echo "${libraries:-(none)}"
echo
echo "Exports of $1:"
echo "${exports:-(none)}"
echo
echo "Imports of $1:"
echo "${imports:-(none)}"
echo

status=0

echo "Test result:"

if echo "$libraries" | grep -q -v -E "$allowed_libraries|^$"; then
	echo "Error: found a library beyond the C standard library."
	status=1
fi

if [ "$(echo "$exports" | grep -c .)" -ne "$export_count" ]; then
	echo "Error: expected $export_count exported symbols."
	status=1
fi

if echo "$exports" | grep -q -v -E "oqmc|^$"; then
	echo "Error: found an exported symbol that is not part of the C ABI."
	status=1
fi

if echo "$imports" | grep -q -v -E "$allowed_symbols|^$"; then
	echo "Warning: found an imported symbol that is not expected."
	# just warn about unexpected symbols
fi

if [ "$status" -eq 0 ]; then
	echo "All checks passed."
fi

exit $status
