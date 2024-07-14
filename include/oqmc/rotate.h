// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/// @file
/// @details Bit rotatation function implementations. These can be used to
/// extract new random values from already estisting hash or rng numbers.

#pragma once

#include "gpu.h"

#include <cstdint>

namespace oqmc
{

/// Rotate bits in an integer value.
/// Offset bits in a 32 bit integer a given distance, while wrapping the bits so
/// that the value remains the same every distance multiplier of 32.
///
/// @param [in] value Initial integer of bits.
/// @param [in] distance Distance to offset.
/// @return Input value after bit rotation.
OQMC_HOST_DEVICE constexpr std::uint32_t rotateBits(std::uint32_t value,
                                                    std::uint32_t distance)
{
	return value >> (+distance & 31) | value << (-distance & 31);
}

/// Rotate bytes in an integer value.
/// Offset bytes in a 4 byte integer a given distance, while wrapping the bytes
/// so that the value remains the same every distance multiplier of 4.
///
/// @param [in] value Initial integer of bits.
/// @param [in] distance Distance to offset.
/// @return Input value after byte rotation.
OQMC_HOST_DEVICE constexpr std::uint32_t rotateBytes(std::uint32_t value,
                                                     int distance)
{
	return rotateBits(value, distance * 8);
}

} // namespace oqmc
