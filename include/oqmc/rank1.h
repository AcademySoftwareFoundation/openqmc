// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/**
 * @file
 * @details An implementation of a rank 1 lattice as described in 'Weighted
 * Compound Integration Rules with Higher Order Convergence for all N' by Fred
 * J. Hickernell, et al., made progressive with a radical inversion of the
 * sample index.
 */

#pragma once

#include "gpu.h"
#include "pcg.h"
#include "permute.h"

#include <cassert>
#include <cstdint>

namespace oqmc
{

/**
 * @brief Rotate an integer a given distance.
 * @details Given a 32 bit unsigned integer value, offset the value a given
 * distance, and rely on integer overflow for the value to wrap around. When
 * applied to elements in a lattice, this represents a toroidal shift or
 * rotation, upon the range of representable values. When the distance is
 * constant for all elements, this can be used to efficiently randomise the
 * values.
 *
 * @param [in] value Integer value to offset.
 * @param [in] distance Distance to offset value.
 * @return Rotated integer value.
 */
OQMC_HOST_DEVICE constexpr std::uint32_t rotate(std::uint32_t value,
                                                std::uint32_t distance)
{
	return value + distance;
}

/**
 * @brief Compute a rank 1 lattice value at an index with reversed bits.
 * @details Given a 32 bit index, where the order of bits in the index have
 * been reversed, compute a rank 1 lattice value to 32 bits of precision for a
 * given dimension. Dimensions must be within the range [0, 4).
 *
 * @param [in] index Bit reversed index of element.
 * @param [in] dimension Dimension of rank 1 lattice.
 * @return Rank 1 lattice value.
 */
OQMC_HOST_DEVICE constexpr std::uint32_t
latticeReversedIndex(std::uint32_t index, int dimension)
{
	assert(dimension >= 0);
	assert(dimension <= 3);

	// clang-format off
	constexpr int lattice[4] = {
		1,
		364981,
		245389,
		97823,
	};
	// clang-format on

	return lattice[dimension] * index;
}

/**
 * @brief Compute a randomised rank 1 lattice value.
 * @details Given an index and a patternId, compute a rank 1 lattice value. The
 * index will be shuffled in a manner that is progressive friendly. The value
 * can be multi-dimensional. For a given lattice, the patternId value must
 * be constant.
 *
 * @tparam Depth Dimensional space of output, up to 4 dimensions.

 * @param [in] index Input index of lattice value.
 * @param [in] patternId Seed to randomise the lattice.
 * @param [out] sample Randomised lattice value.
 */
template <int Depth>
OQMC_HOST_DEVICE constexpr void
shuffledRotatedLattice(std::uint32_t index, std::uint32_t patternId,
                       std::uint32_t sample[Depth])
{
	static_assert(Depth >= 1, "Pattern depth is greater or equal to one.");
	static_assert(Depth <= 4, "Pattern depth is less or equal to four.");

	index = reverseAndShuffle(index, pcg::output(patternId));

	for(int i = 0; i < Depth; ++i)
	{
		sample[i] = latticeReversedIndex(index, i);
		sample[i] = rotate(sample[i], pcg::rng(patternId));
	}
}

} // namespace oqmc
