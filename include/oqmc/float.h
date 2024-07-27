// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/// @file
/// @details Functionality around floating point operations, such as conversion
/// from integer to floating point representation.

#pragma once

#include "gpu.h"

#include <cmath>
#include <cstdint>

namespace oqmc
{

constexpr auto floatOneOverUintMax = 2.3283064365386962890625e-10f; ///< 0x1p-32
constexpr auto floatOneMinusEpsilon = 0.999999940395355224609375f;  ///< max flt

/// Convert an integer into a [0, 1) float.
///
/// Given any representable 32 bit unsigned integer, scale the value into a [0,
/// 1) floating point representation. Note that this operation is lossy and may
/// not be reversible.
///
/// @param [in] value Input integer value within the range [0, 2^32).
/// @return Floating point number within the range [0, 1).
OQMC_HOST_DEVICE inline float uintToFloat(std::uint32_t value)
{
	// This method is inspired by the method used by Matt Pharr in PBRT v4. It
	// has the undesirable property of floating point values rounding up to the
	// nearest representable number to reduce error. This gives the potential
	// for an output equal to one, requiring a min operation. However, this was
	// considered the best tradeoff when compared to other methods.

	// Marc Reynolds gives an option where 8 bits of precision is first removed
	// (http://marc-b-reynolds.github.io/distribution/2017/01/17/DenseFloat.html)
	// prior to division. This removes the need for any min operations, but also
	// looses us a good amount of precision sub one half.

	// An alternative algorithm using a clz operation is detailed in Section 2.1
	// of 'Quasi-Monte Carlo Algorithms (not only) for Graphics Software'. This
	// optimally maps the unsinged integer bits to the floating point exponent
	// and mantissa. However, after benchmarking this alternative method the
	// cost when drawing samples was a 2x time increase for some samplers.

	return std::fmin(value * floatOneOverUintMax, floatOneMinusEpsilon);
}

} // namespace oqmc
