// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/**
 * @file
 * @details Functionality around floating point operations, such as conversion
 * from integer to floating point representation.
 */

#pragma once

#include "gpu.h"

#include <cmath>
#include <cstdint>

namespace oqmc
{

constexpr auto floatOneOverUintMax = 2.3283064365386963e-10f; ///< Integer norm.
constexpr auto floatOneMinusEpsilon = 0.99999994f; ///< Max limit for [0, 1).

/**
 * @brief Convert an integer into a [0, 1) float.
 * @details Given any representable 32 bit unsigned integer, scale the value
 * into a [0, 1) floating point representation. Note that this operation is
 * lossy and may not be reversible.
 *
 * @param [in] value Input integer value within the range [0, 2^32).
 * @return Floating point number within the range [0, 1).
 */
OQMC_HOST_DEVICE inline float uintToFloat(std::uint32_t value)
{
	return std::fmin(value * floatOneOverUintMax, floatOneMinusEpsilon);
}

} // namespace oqmc
