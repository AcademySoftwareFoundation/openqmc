# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

#!/bin/bash

git ls-files | grep -E '\.(cpp)$' | xargs clang-tidy -p build/compile_commands.json --warnings-as-errors='*' --quiet
git ls-files | grep -E '\.(py)$' | xargs ruff check --quiet --show-source
