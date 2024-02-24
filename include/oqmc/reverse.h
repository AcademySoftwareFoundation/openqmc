// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/**
 * @file
 * @details Functions to reverse the order of bits in integer data types. This
 * operation is also known as a radical inversion or a Van der Corput sequence.
 */

#pragma once

#include "gpu.h"

#include <cstdint>

namespace oqmc
{

/**
 * @brief Reverse bits of an unsigned 32 bit integer.
 * @details Given a 32 bit unsigned integer value, reverse the order of bits so
 * that the most significant bits become the least significant, and vise versa.
 *
 * @param [in] value Integer value to reverse.
 * @return Reversed integer value.
 */
OQMC_HOST_DEVICE constexpr std::uint32_t reverseBits32(std::uint32_t value)
{
#if defined(__CUDA_ARCH__)
	value = __brev(value);
#else
	value = (((value & 0xaaaaaaaa) >> 1) | ((value & 0x55555555) << 1));
	value = (((value & 0xcccccccc) >> 2) | ((value & 0x33333333) << 2));
	value = (((value & 0xf0f0f0f0) >> 4) | ((value & 0x0f0f0f0f) << 4));

#if defined(_MSC_VER)
	value = (((value & 0xff00ff00) >> 8) | ((value & 0x00ff00ff) << 8));
	value = ((value >> 16) | (value << 16));
#else
	value = __builtin_bswap32(value);
#endif
#endif

	return value;
}

/**
 * @brief Reverse bits of an unsigned 16 bit integer.
 * @details Given a 16 bit unsigned integer value, reverse the order of bits so
 * that the most significant bits become the least significant, and vise versa.
 *
 * @param [in] value Integer value to reverse.
 * @return Reversed integer value.
 */
OQMC_HOST_DEVICE constexpr std::uint16_t reverseBits16(std::uint16_t value)
{
#if defined(__CUDA_ARCH__)
	value = __brev(value) >> 16;
#else
	value = (((value & 0xaaaaaaaa) >> 1) | ((value & 0x55555555) << 1));
	value = (((value & 0xcccccccc) >> 2) | ((value & 0x33333333) << 2));
	value = (((value & 0xf0f0f0f0) >> 4) | ((value & 0x0f0f0f0f) << 4));

#if defined(_MSC_VER)
	value = ((value >> 8) | (value << 8));
#else
	value = __builtin_bswap16(value);
#endif
#endif

	return value;
}

} // namespace oqmc
