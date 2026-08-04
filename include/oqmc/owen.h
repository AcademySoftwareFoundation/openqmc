// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/// @file
/// @details An efficient implementation of Owen scrambled SZ sequences. This
/// can be used to construct higher level sampler types. The method uses Brent
/// Burley's hash based 'Practical Hash-based Owen Scrambling' construction with
/// added optimisations.

#pragma once

#include "gpu.h"
#include "permute.h"
#include "reverse.h"
#include "rotate.h"

#include <cassert>
#include <cstdint>

namespace oqmc
{

#if !defined(__CUDA_ARCH__)

/// Compute SZ sequence value at an index with pre-reversed bits.
///
/// As oqmc::sobolReversedIndex, but the leading bit reversal has already been
/// applied to the index. Splitting the reversal out lets a draw call reverse
/// the shared index once instead of once per dimension (Chris Kulla, PR #97).
///
/// @param [in] index Pre-reversed, bit reversed index of element.
/// @param [in] dimension Dimension of sequence.
/// @return SZ sequence value.
inline std::uint16_t sobolReversedIndexShared(std::uint16_t index,
                                              int dimension)
{
	assert(dimension >= 0);
	assert(dimension <= 3);

	// Each matrix factors into shift-mask-xor steps (Ahmed 2024, eq. 18),
	// conjugated with reversed masks and right shifts to consume the
	// pre-reversed index. Steps emitted by the matrices cli tool in
	// src/tools/cli/matrices.cpp.
	switch(dimension)
	{
	case 1:
		index ^= static_cast<std::uint16_t>((index & 0xff00) >> 8);
		index ^= static_cast<std::uint16_t>((index & 0xf0f0) >> 4);
		index ^= static_cast<std::uint16_t>((index & 0xcccc) >> 2);
		index ^= static_cast<std::uint16_t>((index & 0xaaaa) >> 1);
		break;
	case 2:
		index ^= static_cast<std::uint16_t>((index & 0xaa00) >> 9);
		index ^= static_cast<std::uint16_t>((index & 0xaa00) >> 8);
		index ^= static_cast<std::uint16_t>((index & 0x5500) >> 7);
		index ^= static_cast<std::uint16_t>((index & 0x4040) >> 6);
		index ^= static_cast<std::uint16_t>((index & 0xa0a0) >> 5);
		index ^= static_cast<std::uint16_t>((index & 0x5050) >> 4);
		index ^= static_cast<std::uint16_t>((index & 0xd8d8) >> 3);
		index ^= static_cast<std::uint16_t>((index & 0xcccc) >> 2);
		index ^= static_cast<std::uint16_t>((index & 0xeeee) >> 1);
		break;
	case 3:
		// Dimension 3 is the dimension 1 program applied to dimension 2
		// (Ahmed et al. 2025, eq. 13), which is five steps cheaper than its
		// own program. A draw reuses the dimension 2 value it already has.
		return sobolReversedIndexShared(sobolReversedIndexShared(index, 2), 1);
	default:
		break;
	}

	return index;
}

#endif

/// Compute SZ sequence value at an index with reversed bits.
///
/// Given a 16 bit index, where the order of bits in the index have been
/// reversed, compute an SZ sequence value to 16 bits of precision for a given
/// dimension. Dimensions must be within the range [0, 4).
///
/// @param [in] index Bit reversed index of element.
/// @param [in] dimension Dimension of sequence.
/// @return SZ sequence value.
OQMC_HOST_DEVICE inline std::uint16_t sobolReversedIndex(std::uint16_t index,
                                                         int dimension)
{
	assert(dimension >= 0);
	assert(dimension <= 3);

#if defined(__CUDA_ARCH__)

	// Prefer left shifts on GPU since they are more efficient than right
	// shifts. Reversal must be applied to the result.
	switch(dimension)
	{
	case 1:
		index ^= static_cast<std::uint16_t>((index & 0x00ff) << 8);
		index ^= static_cast<std::uint16_t>((index & 0x0f0f) << 4);
		index ^= static_cast<std::uint16_t>((index & 0x3333) << 2);
		index ^= static_cast<std::uint16_t>((index & 0x5555) << 1);
		break;
	case 2:
		index ^= static_cast<std::uint16_t>((index & 0x0055) << 9);
		index ^= static_cast<std::uint16_t>((index & 0x0055) << 8);
		index ^= static_cast<std::uint16_t>((index & 0x00aa) << 7);
		index ^= static_cast<std::uint16_t>((index & 0x0202) << 6);
		index ^= static_cast<std::uint16_t>((index & 0x0505) << 5);
		index ^= static_cast<std::uint16_t>((index & 0x0a0a) << 4);
		index ^= static_cast<std::uint16_t>((index & 0x1b1b) << 3);
		index ^= static_cast<std::uint16_t>((index & 0x3333) << 2);
		index ^= static_cast<std::uint16_t>((index & 0x7777) << 1);
		break;
	case 3:
		index ^= static_cast<std::uint16_t>((index & 0x0055) << 9);
		index ^= static_cast<std::uint16_t>((index & 0x00aa) << 8);
		index ^= static_cast<std::uint16_t>((index & 0x00aa) << 7);
		index ^= static_cast<std::uint16_t>((index & 0x0202) << 6);
		index ^= static_cast<std::uint16_t>((index & 0x0505) << 5);
		index ^= static_cast<std::uint16_t>((index & 0x0505) << 4);
		index ^= static_cast<std::uint16_t>((index & 0x1b1b) << 3);
		index ^= static_cast<std::uint16_t>((index & 0x2222) << 2);
		index ^= static_cast<std::uint16_t>((index & 0x2222) << 1);
		break;
	default:
		break;
	}

	return reverseBits16(index);

#else

	// Reversed masks and right shifts put the reversal on the shared input.
	// This optimization hoists the reversal out of a draw
	// (Chris Kulla, PR #97).
	return sobolReversedIndexShared(reverseBits16(index), dimension);

#endif
}

/// Permute an input integer and reverse the bits.
///
/// Given an input integer value, perform a Laine and Karras style permutation
/// and reverse the resulting bits. The permutation can be randomised with a
/// given seed value. This will be equivalent to an Owen scramble when the input
/// bits of the integer are already reversed.
///
/// @param [in] value Input integer value.
/// @param [in] seed Seed to change the permutation.
/// @return Permuted, reversed value.
OQMC_HOST_DEVICE constexpr std::uint32_t scrambleAndReverse(std::uint32_t value,
                                                            std::uint32_t seed)
{
	value = laineKarrasPermutation(value, seed);
	value = reverseBits32(value);

	return value;
}

/// Compute a randomised SZ sequence value.
///
/// Given an index and a seed, compute an Owen scrambled SZ sequence value.
/// The index will be shuffled in a manner that is progressive friendly. The
/// value can be multi-dimensional. For a given sequence, the seed value must be
/// constant. An index greater than 2^16 will repeat values.
///
/// @tparam Depth Dimensional space of output, up to 4 dimensions.
/// @param [in] index Input index of sequence value.
/// @param [in] seed Seed to randomise the sequence.
/// @param [out] sample Randomised sequence value.
template <int Depth>
OQMC_HOST_DEVICE inline void shuffledScrambledSobol(std::uint32_t index,
                                                    std::uint32_t seed,
                                                    std::uint32_t sample[Depth])
{
	static_assert(Depth >= 1, "Pattern depth is greater or equal to one.");
	static_assert(Depth <= 4, "Pattern depth is less or equal to four.");

	index = reverseAndShuffle(index, seed);

#if defined(__CUDA_ARCH__)

	for(int i = 0; i < Depth; ++i)
	{
		sample[i] = sobolReversedIndex(index >> 16, i);
		sample[i] = scrambleAndReverse(sample[i], rotateBytes(seed, i));
	}

#else

	// Reverse the shared index once for all dimensions (Kulla, PR #97), and
	// evaluate dimension 3 from dimension 2 (Ahmed et al. 2025, eq. 13).
	const auto reversed = reverseBits16(index >> 16);

	std::uint16_t value[4];
	for(int i = 0; i < Depth; ++i)
	{
		value[i] = i == 3 ? sobolReversedIndexShared(value[2], 1)
		                  : sobolReversedIndexShared(reversed, i);
		sample[i] = scrambleAndReverse(value[i], rotateBytes(seed, i));
	}

#endif
}

} // namespace oqmc
