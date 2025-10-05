// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/// @file
/// @details An implementation for hash based permutations. A requirement when
/// constructing scrambling or randomisation of progressive sequences. This is a
/// variant of the hash originally published by Samuli Laine and Tero Karras in
/// 'Stratified Sampling for Stochastic Transparency'. But was then later
/// improved upon by Nathan Vegdahl in an article at
/// https://psychopath.io/post/2021_01_30_building_a_better_lk_hash.

#pragma once

#include "gpu.h"
#include "reverse.h"

#include <cstdint>

namespace oqmc
{

/// Laine and Karras style permutation.
///
/// Given an unsigned integer number, permute the bits so that lower bits effect
/// higher bits, but not the other way around. When combined with a reversal of
/// bits before and after, this forms an efficient hash based owen scrambling
/// scheme.
///
/// @ingroup utilities
/// @param [in] value Integer value to permute.
/// @param [in] seed Seed value to randomise the permutation.
/// @return Permuted output value.
OQMC_HOST_DEVICE constexpr std::uint32_t
laineKarrasPermutation(std::uint32_t value, std::uint32_t seed)
{
	value ^= value * 0x3d20adea;
	value += seed;
	value *= (seed >> 16) | 1;
	value ^= value * 0x05526c56;
	value ^= value * 0x53a22864;

	return value;
}

/// Reverse input bits and shuffle order.
///
/// Given an unsigned integer number, reverse the bit order and then perform a
/// Laine and Karras style permutation.
///
/// @ingroup utilities
/// @param [in] value Integer value to reverse and permute.
/// @param [in] seed Seed value to randomise the permutation.
/// @return Reversed and permuted output value.
OQMC_HOST_DEVICE constexpr std::uint32_t reverseAndShuffle(std::uint32_t value,
                                                           std::uint32_t seed)
{
	value = reverseBits32(value);
	value = laineKarrasPermutation(value, seed);

	return value;
}

/// Compute a hash based owen scramble.
///
/// Given an unsigned integer, use a hash based technique to perform an owen
/// scramble. This can be used to scramble a value, or to shuffle the order of a
/// sequence in a progressive friendly manner.
///
/// @ingroup utilities
/// @param [in] value Integer value to be scrambled.
/// @param [in] seed Seed value to randomise the scramble.
/// @return Scrambled output value.
OQMC_HOST_DEVICE constexpr std::uint32_t shuffle(std::uint32_t value,
                                                 std::uint32_t seed)
{
	value = reverseAndShuffle(value, seed);
	value = reverseBits32(value);

	return value;
}

} // namespace oqmc
