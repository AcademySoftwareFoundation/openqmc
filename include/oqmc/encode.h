// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/// @file
/// @details Functions to encode (compress) and decode (decompress) key data
/// into a smaller memory footprint. This can be used to efficiently store pixel
/// coordinate information.

#pragma once

#include "gpu.h"

#include <cstdint>

namespace oqmc
{

/// Key to encode pixel coordinates.
///
/// This structure stores integer coordinate information for each axis of a 3
/// dimensional array.
struct EncodeKey
{
	int x; ///< x axis coordinate.
	int y; ///< y axis coordinate.
	int z; ///< z axis coordinate.
};

/// Encode a key value into 16 bits.
///
/// Given a coordinate key and a given precision for each axis, encode the
/// values into a single 16 bit integer value. This can be a lossy operation.
/// The sum of all precisions must be equal to or less than 16 bits. Decode the
/// values using the decodeBits16 function.
///
/// @tparam XBits Requested precision along the X axis.
/// @tparam YBits Requested precision along the Y axis.
/// @tparam ZBits Requested precision along the Z axis.
/// @param [in] key Key of coordinates to encode.
/// @return Encoded 16 bit integer.
template <int XBits, int YBits, int ZBits>
OQMC_HOST_DEVICE inline std::uint16_t encodeBits16(EncodeKey key)
{
	constexpr auto sum = XBits + YBits + ZBits;

	static_assert(sum <= 16, "Precision sum must be equal or less than 16");

	constexpr auto maskX = (1 << XBits) - 1;
	constexpr auto maskY = (1 << YBits) - 1;
	constexpr auto maskZ = (1 << ZBits) - 1;

	constexpr auto offsetX = 0;
	constexpr auto offsetY = XBits;
	constexpr auto offsetZ = XBits + YBits;

	std::uint16_t value = 0;
	value |= (key.x & maskX) << offsetX;
	value |= (key.y & maskY) << offsetY;
	value |= (key.z & maskZ) << offsetZ;

	return value;
}

/// Decode a key value back into a key.
///
/// Given a encoded 16 bit integer value and a given precision for each axis,
/// decode the values into a coordinate key. This can be a lossy operation. The
/// sum of all precisions must be equal to or less than 16 bits. Encode the
/// values using the encodeBits16 function.
///
/// @tparam XBits Requested precision along the X axis.
/// @tparam YBits Requested precision along the Y axis.
/// @tparam ZBits Requested precision along the Z axis.
/// @param [in] value Encoded 16 bit integer.
/// @return Key of encoded coordinates.
template <int XBits, int YBits, int ZBits>
OQMC_HOST_DEVICE inline EncodeKey decodeBits16(std::uint16_t value)
{
	constexpr auto sum = XBits + YBits + ZBits;

	static_assert(sum <= 16, "Precision sum must be equal or less than 16");

	constexpr auto maskX = (1 << XBits) - 1;
	constexpr auto maskY = (1 << YBits) - 1;
	constexpr auto maskZ = (1 << ZBits) - 1;

	constexpr auto offsetX = 0;
	constexpr auto offsetY = XBits;
	constexpr auto offsetZ = XBits + YBits;

	const auto x = (value >> offsetX) & maskX;
	const auto y = (value >> offsetY) & maskY;
	const auto z = (value >> offsetZ) & maskZ;

	return {x, y, z};
}

} // namespace oqmc
