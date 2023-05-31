// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#pragma once

#include "abi.h"
#include "vector.h"

// NOLINTNEXTLINE: C style naming
OQMC_CABI bool oqmc_trace(const char* name, const char* scene, int width,
                          int height, int frame, int numPixelSamples,
                          int numLightSamples, int maxDepth, int maxOpacity,
                          float3* image);
