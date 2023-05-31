# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

#!/bin/bash

grepCmd="grep -E '\.(cpp|h)$'"
xargsCmd="xargs clang-format -i"

git ls-files | eval "$grepCmd" | eval "$xargsCmd"
