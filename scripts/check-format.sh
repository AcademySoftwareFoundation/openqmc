# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

#!/bin/bash
set -e

clang-format --version
git ls-files | grep -E '\.(cpp|h)$' | xargs clang-format --dry-run -Werror

ruff --version
git ls-files | grep -E '\.(py)$' | xargs ruff format --diff --quiet
