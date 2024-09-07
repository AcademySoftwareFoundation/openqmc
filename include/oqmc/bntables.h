// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/// @file
/// @details Pre-computed and optimised blue noise tables used to decorrelate
/// between pixels, and extend the base sampler implementations with blue noise
/// properties. A generalised method extending 'Lessons Learned and Improvements
/// when Building Screen-Space Samplers with Blue-Noise Error Distribution.'
/// by Laurent Belcour and Eric Heitz was used to optimise the tables. Lookups
/// for the table can apply constant random shifts for different domains,
/// allowing a single table to be re-used for N domains.

#pragma once

#include "encode.h"
#include "gpu.h"

#include <cstdint>

namespace oqmc
{

namespace bntables
{

/// Return type for table value.
///
/// This type composes both a key and a rank value as a pair. These values can
/// then be used to randomise a sequence.
struct TableReturnValue
{
	std::uint32_t key;  ///< key value to randomise.
	std::uint32_t rank; ///< rank value to shuffle.
};

/// Lookup value pair from table.
///
/// Given an encoded pixel coordinate and an encoded pixel shift, decode the
/// values and add the shift to the coordinate to compute an index. Using the
/// index lookup a key and rank value pair from the input tables.
///
/// @tparam XBits Precision along the X axis used for encoding.
/// @tparam YBits Precision along the Y axis used for encoding.
/// @tparam ZBits Precision along the Z axis used for encoding.
/// @param [in] pixel Encoded pixel coordinate for lookup.
/// @param [in] shift Encoded pixel shift for lookup.
/// @param [in] keyTable Table of key values.
/// @param [in] rankTable Table of rank values.
/// @return Key and rank pair from lookup.
template <int XBits, int YBits, int ZBits>
OQMC_HOST_DEVICE constexpr TableReturnValue
tableValue(std::uint16_t pixel, std::uint16_t shift,
           const std::uint32_t keyTable[], const std::uint32_t rankTable[])
{
	const auto pixelOffset = decodeBits16<XBits, YBits, ZBits>(pixel);
	const auto shiftOffset = decodeBits16<XBits, YBits, ZBits>(shift);

	const auto x = pixelOffset.x + shiftOffset.x;
	const auto y = pixelOffset.y + shiftOffset.y;
	const auto z = pixelOffset.z + shiftOffset.z;
	const auto index = encodeBits16<XBits, YBits, ZBits>({x, y, z});

	return {
	    keyTable[index],
	    rankTable[index],
	};
}

constexpr auto xBits = 6;                           ///< 64 pixels in x.
constexpr auto yBits = 6;                           ///< 64 pixels in y.
constexpr auto zBits = 4;                           ///< 16 pixels in time.
constexpr auto size = 1 << (xBits + yBits + zBits); ///< 2^16 table size.

static_assert(xBits == yBits,
              "Optimisation tables have equal resolution in x and y");

// Following optimised blue noise randomisation values were generated using the
// optimise cli tool found in the source file src/tools/lib/optimise.cpp.

namespace pmj
{

#if defined(OQMC_ENABLE_BINARY)

/// Optimised blue noise key table for pmj.
extern const std::uint32_t keyTable[size];

/// Optimised blue noise rank table for pmj.
extern const std::uint32_t rankTable[size];

#else

/// Optimised blue noise key table for pmj.
constexpr std::uint32_t keyTable[] = {
#include "data/pmj/keys.txt"
};

/// Optimised blue noise rank table for pmj.
constexpr std::uint32_t rankTable[] = {
#include "data/pmj/ranks.txt"
};

#endif

} // namespace pmj

namespace sobol
{

#if defined(OQMC_ENABLE_BINARY)

/// Optimised blue noise key table for sobol.
extern const std::uint32_t keyTable[size];

/// Optimised blue noise rank table for sobol.
extern const std::uint32_t rankTable[size];

#else

/// Optimised blue noise key table for sobol.
constexpr std::uint32_t keyTable[] = {
#include "data/sobol/keys.txt"
};

/// Optimised blue noise rank table for sobol.
constexpr std::uint32_t rankTable[] = {
#include "data/sobol/ranks.txt"
};

#endif

} // namespace sobol

namespace lattice
{

#if defined(OQMC_ENABLE_BINARY)

/// Optimised blue noise key table for lattice.
extern const std::uint32_t keyTable[size];

/// Optimised blue noise rank table for lattice.
extern const std::uint32_t rankTable[size];

#else

/// Optimised blue noise key table for lattice.
constexpr std::uint32_t keyTable[] = {
#include "data/lattice/keys.txt"
};

/// Optimised blue noise rank table for lattice.
constexpr std::uint32_t rankTable[] = {
#include "data/lattice/ranks.txt"
};

#endif

} // namespace lattice

} // namespace bntables

} // namespace oqmc
