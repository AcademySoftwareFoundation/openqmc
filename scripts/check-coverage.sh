# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

#!/bin/bash
set -e

LLVM_PROFILE_FILE='build/coverage.profraw' build/src/tests/tests
llvm-profdata merge --sparse --output build/coverage.profdata build/coverage.profraw
llvm-cov report --instr-profile build/coverage.profdata build/src/tests/tests include
