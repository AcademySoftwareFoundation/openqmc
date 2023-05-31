// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#pragma once

#include "abi.h"

// NOLINTNEXTLINE: C style naming
OQMC_CABI bool oqmc_benchmark(const char* sampler, const char* measurement,
                              int nsamples, int ndims, int* out);
