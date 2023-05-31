# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

#!/bin/bash

grepCmd="grep -E '\.cpp$' | grep -v -E '^cmake/examples/'"
xargsCmd="xargs clang-tidy -p build/compile_commands.json --quiet --fix"

git ls-files | eval "$grepCmd" | eval "$xargsCmd"
