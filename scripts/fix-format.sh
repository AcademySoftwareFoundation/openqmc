# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

#!/bin/bash

git ls-files | grep -E '\.(cpp|h)$' | xargs clang-format -i
git ls-files | grep -E '\.(py)$' | xargs ruff format --quiet
