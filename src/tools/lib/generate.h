// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#pragma once

#include "abi.h"

// NOLINTNEXTLINE: C style naming
OQMC_CABI bool oqmc_generate(const char* name, int nsequences, int nsamples,
                             int ndims, float* out);
