// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#pragma once

#include "abi.h"

#include <stdint.h> // NOLINT: can be a C header

// NOLINTNEXTLINE: C style naming
OQMC_CABI bool oqmc_optimise(const char* name, int ntests, int niterations,
                             int nsamples, int resolution, int seed,
                             uint32_t* keys, uint32_t* ranks, float* estimates,
                             float* frequencies);
