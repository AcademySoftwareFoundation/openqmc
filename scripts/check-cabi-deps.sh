#!/usr/bin/env bash

# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

# Check that the shared C ABI library depends on nothing beyond the C standard
# library (libc), so that C consumers don't acquire other dependencies, such as
# the C++ runtime.

set -eo pipefail

case "$(uname -s)" in
Linux)
	dependencies=$(readelf -d "$1" | sed -n 's/.*Shared library: \[\(.*\)\]/\1/p')
	allowed='^(libc\.so\.6|ld-linux.*\.so\.[0-9]+)$' # Loader is part of glibc
	;;
Darwin)
	dependencies=$(otool -L "$1" | tail -n +3 | awk '{print $1}')
	allowed='^/usr/lib/libSystem\.B\.dylib$'
	;;
*) echo "Unsupported platform: $(uname -s)" >&2 && exit 2 ;;
esac

echo "Dependencies of $1:"
echo "${dependencies:-(none)}"

if echo "$dependencies" | grep -q -v -E "$allowed|^$"; then
	echo "Found a dependency beyond the C standard library."
	exit 1
fi
