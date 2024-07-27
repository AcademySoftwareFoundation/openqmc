// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/// @file
/// @details Table lookup functionality. This can be paired with various
/// pre-initialised sample sequence methods to randomise a sequence lookup. As a
/// result a single initialised table can be reused to save on initialisation
/// and storage costs.

#pragma once

#include "gpu.h"
#include "permute.h"
#include "rotate.h"

#include <cassert>
#include <cstdint>

namespace oqmc
{

/// Random digit scramble an element in a sequence.
///
/// Given a value and a random number, efficiently randomise the value using the
/// random digit scramble method from Kollig and Keller in 'Efficient
/// Multidimensional Sampling'. The implementation just requires a single XOR
/// operation, which although fast will not remove any pre-existing structure
/// present in a sequence. The seed input must be constant for a given sequence.
///
/// @param [in] value Sequence element.
/// @param [in] hash Random number.
/// @return Randomised element.
OQMC_HOST_DEVICE constexpr std::uint32_t
randomDigitScramble(std::uint32_t value, std::uint32_t hash)
{
	return value ^ hash;
}

/// Compute a randomised value from a pre-computed table.
///
/// Given an index and a seed, compute an scrambled sequence value. The index
/// will be shuffled in a manner that is progressive friendly. The value can be
/// multi-dimensional. For a given sequence, the seed value must be constant.
/// Table element size must be equal to or greater than 2^16. An index greater
/// than 2^16 will reuse table samples.
///
/// @tparam Table Dimensional size of input table.
/// @tparam Depth Dimensional space of output, up to 4 dimensions.
/// @param [in] index Input index of sequence value.
/// @param [in] hash Hashed seed to randomise the sequence.
/// @param [in] table Pre-computed input table.
/// @param [out] sample Randomised sequence value.
/// @pre Table input must be pre-computed using an initialisation function.
template <int Table, int Depth>
OQMC_HOST_DEVICE inline void
shuffledScrambledLookup(std::uint32_t index, std::uint32_t hash,
                        const std::uint32_t table[][Table],
                        std::uint32_t sample[Depth])
{
	static_assert(Table >= Depth, "Table size is greater or equal to Depth.");
	static_assert(Depth >= 1, "Pattern depth is greater or equal to one.");
	static_assert(Depth <= 4, "Pattern depth is less or equal to four.");

	index = shuffle(index, hash);

	for(int i = 0; i < Depth; ++i)
	{
		constexpr auto indexMask = 0xffff;

		sample[i] = table[index & indexMask][i];
		sample[i] = randomDigitScramble(sample[i], rotateBytes(hash, i));
	}
}

} // namespace oqmc
