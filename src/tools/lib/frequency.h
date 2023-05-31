// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#pragma once

#include "abi.h"

// NOLINTNEXTLINE: C style naming
OQMC_CABI bool oqmc_frequency_continuous(int nsequences, int nsamples,
                                         int ndims, int depthA, int depthB,
                                         int resolution, const float* in,
                                         float* out);

// NOLINTNEXTLINE: C style naming
OQMC_CABI bool oqmc_frequency_discrete_1d(int resolution, const float* inReal,
                                          const float* inImaginary,
                                          float* outReal, float* outImaginary);

// NOLINTNEXTLINE: C style naming
OQMC_CABI bool oqmc_frequency_discrete_2d(int resolution, const float* in,
                                          float* out);

// NOLINTNEXTLINE: C style naming
OQMC_CABI bool oqmc_frequency_discrete_3d(int resolution, int depth,
                                          const float* in, float* out);
