# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

#!/bin/bash
set -e

clang-tidy --version
git ls-files | grep -E '\.(cpp)$' | grep -v -E '^cmake/examples/' | xargs clang-tidy -p build/compile_commands.json --warnings-as-errors='*' --quiet

ruff --version
git ls-files | grep -E '\.(py)$' | xargs ruff check --quiet

ty --version
git ls-files | grep -E '\.(py)$' | xargs ty check --quiet
