# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

#!/bin/bash

clang-format --version
git ls-files | grep -E '\.(cpp|h)$' | xargs clang-format -i

ruff --version
git ls-files | grep -E '\.(py)$' | xargs ruff format --quiet
