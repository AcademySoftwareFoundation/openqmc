// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/**
 * @file
 * @details Functionality around floating point operations, such as conversion
 * from integer to floating point representation.
 */

#pragma once

#include "gpu.h"

#include <cassert>
#include <cstdint>
#include <cstring>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace oqmc
{

/**
 * @brief Cast unsigned integer bits to a float.
 * @details Given a 32 bit unsigned integer cast it to a 32 bit float. Memcpy
 * is defined behaviour and comlies with the C++ standard. This becomes a no-op
 * once optimised by the compiler.
 *
 * @param [in] value Input integer bits to be cast.
 * retrun Bits cast as a floating point number.
 */
OQMC_HOST_DEVICE inline float bitsToFloat(std::uint32_t value)
{
	float out;
	std::memcpy(&out, &value, sizeof(std::uint32_t));

	return out;
}

/**
 * @brief Count zero bits from the most significant digit.
 * @details From the most significant bit, count the number of zeros leading
 * before the first one bit.
 *
 * @param [in] value Input integer bits. Must be greater than zero.
 * @return Number of leading zeros.
 */
OQMC_HOST_DEVICE inline int countLeadingZeros(std::uint32_t value)
{
	assert(value > 0);

#if defined(_MSC_VER)
	auto index = 0ul;
	_BitScanReverse(&index, value);

	return 31 - index;
#endif

#if defined(__CUDA_ARCH__)
	return __clz(value);
#else
	return __builtin_clz(value);
#endif
}

/**
 * @brief Convert an integer into a [0, 1) float.
 * @details Given any representable 32 bit unsigned integer, scale the value
 * into a [0, 1) floating point representation. Note that not all values in the
 * range can be represented, while this is at the same time a lossy operation.
 *
 * For futher information see Section 2.1 in https://arxiv.org/abs/2307.15584
 *'Quasi-Monte Carlo Algorithms (not only) for Graphics Software'.
 *
 * @param [in] value Input integer value within the range [0, 2^32).
 * @return Floating point number within the range [0, 1).
 */
OQMC_HOST_DEVICE inline float uintToFloat(std::uint32_t value)
{
	if(value == 0)
	{
		return 0.0f;
	}

	if(value == 1)
	{
		return bitsToFloat(0x5F << 23);
	}

	const auto clz = countLeadingZeros(value);
	const auto bias = static_cast<std::uint32_t>(127);

	// Shift an extra bit as implicit leading one.
	const auto mantissa = value << (clz + 1);
	const auto exponent = bias - (clz + 1);

	return bitsToFloat(exponent << 23 | mantissa >> 9);
}

} // namespace oqmc
