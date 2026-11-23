#!/usr/bin/env bash

# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

set -e

clang-format --version
git ls-files | grep -E '\.(c|cpp|h)$' | xargs clang-format --dry-run -Werror

ruff --version
git ls-files | grep -E '\.(py)$' | xargs ruff format --diff --quiet

zig version
git ls-files | grep -E '\.(zig|zon)$' | xargs zig fmt --check
