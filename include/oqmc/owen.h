// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/// @file
/// @details An efficient implementation of Owen scrambled sobol sequences. This
/// can be used to construct higher level sampler types. The method uses Brent
/// Burley's hash based 'Practical Hash-based Owen Scrambling' construction with
/// added optimisations.

#pragma once

#include "arch.h"
#include "gpu.h"
#include "pcg.h"
#include "permute.h"
#include "reverse.h"
#include "rotate.h"

#include <cassert>
#include <cstdint>

#if defined(OQMC_ARCH_AVX)
#include <immintrin.h>
#endif

#if defined(OQMC_ARCH_SSE)
#include <emmintrin.h>
#endif

#if defined(OQMC_ARCH_ARM)
#include <arm_neon.h>
#endif

namespace oqmc
{

/// Compute sobol sequence value at an index with reversed bits.
///
/// Given a 16 bit index, where the order of bits in the index have been
/// reversed, compute a sobol sequence value to 16 bits of precision for a given
/// dimension. Dimensions must be within the range [0, 4).
///
/// @param [in] index Bit reversed index of element.
/// @param [in] dimension Dimension of sobol sequence.
/// @return Sobol sequence value.
OQMC_HOST_DEVICE inline std::uint16_t sobolReversedIndex(std::uint16_t index,
                                                         int dimension)
{
	assert(dimension >= 0);
	assert(dimension <= 3);

	if(dimension == 0)
	{
		return reverseBits16(index);
	}

	// clang-format off
	constexpr std::uint16_t masks[16] = {
		0b0000000000000001,
		0b0000000000000010,
		0b0000000000000100,
		0b0000000000001000,
		0b0000000000010000,
		0b0000000000100000,
		0b0000000001000000,
		0b0000000010000000,
		0b0000000100000000,
		0b0000001000000000,
		0b0000010000000000,
		0b0000100000000000,
		0b0001000000000000,
		0b0010000000000000,
		0b0100000000000000,
		0b1000000000000000,
	};

	constexpr std::uint16_t directions[4][16] = {
		{
		0b1000000000000000,
		0b0100000000000000,
		0b0010000000000000,
		0b0001000000000000,
		0b0000100000000000,
		0b0000010000000000,
		0b0000001000000000,
		0b0000000100000000,
		0b0000000010000000,
		0b0000000001000000,
		0b0000000000100000,
		0b0000000000010000,
		0b0000000000001000,
		0b0000000000000100,
		0b0000000000000010,
		0b0000000000000001,
		},

		{
		0b1111111111111111,
		0b0101010101010101,
		0b0011001100110011,
		0b0001000100010001,
		0b0000111100001111,
		0b0000010100000101,
		0b0000001100000011,
		0b0000000100000001,
		0b0000000011111111,
		0b0000000001010101,
		0b0000000000110011,
		0b0000000000010001,
		0b0000000000001111,
		0b0000000000000101,
		0b0000000000000011,
		0b0000000000000001,
		},

		{
		0b1010101000001001,
		0b0111011100000110,
		0b0011100100000011,
		0b0001011000000001,
		0b0000100110101010,
		0b0000011001110111,
		0b0000001100111001,
		0b0000000100010110,
		0b0000000010100011,
		0b0000000001110001,
		0b0000000000111010,
		0b0000000000010111,
		0b0000000000001001,
		0b0000000000000110,
		0b0000000000000011,
		0b0000000000000001,
		},

		{
		0b1010000011000011,
		0b0100000001000001,
		0b0011000000101101,
		0b0001000000011110,
		0b0000101101100111,
		0b0000011110011010,
		0b0000001010100100,
		0b0000000100011011,
		0b0000000011001001,
		0b0000000001000101,
		0b0000000000101110,
		0b0000000000011111,
		0b0000000000001010,
		0b0000000000000100,
		0b0000000000000011,
		0b0000000000000001,
		},
	};
	// clang-format on

	const auto matrix = directions[dimension];

#if defined(OQMC_ARCH_AVX)
	constexpr auto stride = 16;
	const __m256i zero = _mm256_setzero_si256();

	__m256i bits = zero;
	for(int i = 0; i < 16; i += stride)
	{
		const auto maskPtr = reinterpret_cast<const __m256i*>(masks + i);
		const __m256i mask = _mm256_loadu_si256(maskPtr);

		const auto matrixPtr = reinterpret_cast<const __m256i*>(matrix + i);
		const __m256i column = _mm256_loadu_si256(matrixPtr);

		const __m256i masked = _mm256_and_si256(_mm256_set1_epi16(index), mask);
		const __m256i cond = _mm256_cmpeq_epi16(masked, zero);

		const __m256i xored = _mm256_xor_si256(bits, column);

		bits = _mm256_or_si256(_mm256_and_si256(cond, bits),
		                       _mm256_andnot_si256(cond, xored));
	}

	bits = _mm256_xor_si256(bits, _mm256_srli_si256(bits, 2));
	bits = _mm256_xor_si256(bits, _mm256_srli_si256(bits, 4));
	bits = _mm256_xor_si256(bits, _mm256_srli_si256(bits, 8));

	return _mm256_extract_epi16(bits, 0) ^ _mm256_extract_epi16(bits, 8);
#endif

#if defined(OQMC_ARCH_SSE)
	constexpr auto stride = 8;
	const __m128i zero = _mm_setzero_si128();

	__m128i bits = zero;
	for(int i = 0; i < 16; i += stride)
	{
		const auto maskPtr = reinterpret_cast<const __m128i*>(masks + i);
		const __m128i mask = _mm_loadu_si128(maskPtr);

		const auto matrixPtr = reinterpret_cast<const __m128i*>(matrix + i);
		const __m128i column = _mm_loadu_si128(matrixPtr);

		const __m128i masked = _mm_and_si128(_mm_set1_epi16(index), mask);
		const __m128i cond = _mm_cmpeq_epi16(masked, zero);

		const __m128i xored = _mm_xor_si128(bits, column);

		bits = _mm_or_si128(_mm_and_si128(cond, bits),
		                    _mm_andnot_si128(cond, xored));
	}

	bits = _mm_xor_si128(bits, _mm_srli_si128(bits, 2));
	bits = _mm_xor_si128(bits, _mm_srli_si128(bits, 4));
	bits = _mm_xor_si128(bits, _mm_srli_si128(bits, 8));

	return _mm_extract_epi16(bits, 0);
#endif

#if defined(OQMC_ARCH_ARM)
	constexpr auto stride = 8;
	const uint16x8_t zero = vdupq_n_u16(0);

	uint16x8_t bits = zero;
	for(int i = 0; i < 16; i += stride)
	{
		const uint16x8_t mask = vld1q_u16(masks + i);
		const uint16x8_t column = vld1q_u16(matrix + i);

		const uint16x8_t masked = vandq_u16(vdupq_n_u16(index), mask);
		const uint16x8_t cond = vceqq_u16(masked, zero);

		const uint16x8_t xored = veorq_u16(bits, column);

		bits =
		    vorrq_u16(vandq_u16(cond, bits), vandq_u16(vmvnq_u16(cond), xored));
	}

	bits = veorq_u16(bits, vextq_u16(bits, zero, 1));
	bits = veorq_u16(bits, vextq_u16(bits, zero, 2));
	bits = veorq_u16(bits, vextq_u16(bits, zero, 4));

	return vgetq_lane_u16(bits, 0);
#endif

#if defined(OQMC_ARCH_SCALAR)
	std::uint16_t sample = 0;
	for(int i = 0; i < 16; ++i)
	{
		if((index & masks[i]) != 0)
		{
			sample ^= matrix[i];
		}
	}

	return sample;
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

/// Compute a randomised sobol sequence value.
///
/// Given an index and a seed, compute an Owen scrambled sobol sequence value.
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

	for(int i = 0; i < Depth; ++i)
	{
		sample[i] = sobolReversedIndex(index >> 16, i);
		sample[i] = scrambleAndReverse(sample[i], rotateBytes(seed, i));
	}
}

OQMC_HOST_DEVICE constexpr std::uint16_t sobolDimension5(std::uint16_t index)
{
	// clang-format off
	constexpr std::uint16_t masks[16] = {
		0b0000000000000001,
		0b0000000000000010,
		0b0000000000000100,
		0b0000000000001000,
		0b0000000000010000,
		0b0000000000100000,
		0b0000000001000000,
		0b0000000010000000,
		0b0000000100000000,
		0b0000001000000000,
		0b0000010000000000,
		0b0000100000000000,
		0b0001000000000000,
		0b0010000000000000,
		0b0100000000000000,
		0b1000000000000000,
	};

	constexpr std::uint16_t matrix[16] = {
		0b1000000000000000,
		0b0100000000000000,
		0b0010000000000000,
		0b1011000000000000,
		0b1111100000000000,
		0b1101110000000000,
		0b0111101000000000,
		0b1001110100000000,
		0b0101101010000000,
		0b0010111111000000,
		0b1010000101100000,
		0b1111000010110000,
		0b1101101010001000,
		0b0110111111000100,
		0b1000000101100010,
		0b0100000010111011,
	};
	// clang-format on

	std::uint16_t sample = 0;
	for(int i = 0; i < 16; ++i)
	{
		if((index & masks[i]) != 0)
		{
			sample ^= matrix[i];
		}
	}

	return sample;
}

OQMC_HOST_DEVICE constexpr std::uint16_t
sobolDimension5Inv(std::uint16_t sample)
{
	// clang-format off
	constexpr std::uint16_t masks[16] = {
		0b0000000000000001,
		0b0000000000000010,
		0b0000000000000100,
		0b0000000000001000,
		0b0000000000010000,
		0b0000000000100000,
		0b0000000001000000,
		0b0000000010000000,
		0b0000000100000000,
		0b0000001000000000,
		0b0000010000000000,
		0b0000100000000000,
		0b0001000000000000,
		0b0010000000000000,
		0b0100000000000000,
		0b1000000000000000,
	};

	constexpr std::uint16_t masksInversed[16] = {
		0b1000000000000000,
		0b0100000000000000,
		0b0010000000000000,
		0b0001000000000000,
		0b0000100000000000,
		0b0000010000000000,
		0b0000001000000000,
		0b0000000100000000,
		0b0000000010000000,
		0b0000000001000000,
		0b0000000000100000,
		0b0000000000010000,
		0b0000000000001000,
		0b0000000000000100,
		0b0000000000000010,
		0b0000000000000001,
	};

	constexpr std::uint16_t matrix[16] = {
		0b1000000000000000,
		0b0100000000000000,
		0b0010000000000000,
		0b1011000000000000,
		0b1111100000000000,
		0b1101110000000000,
		0b0111101000000000,
		0b1001110100000000,
		0b0101101010000000,
		0b0010111111000000,
		0b1010000101100000,
		0b1111000010110000,
		0b1101101010001000,
		0b0110111111000100,
		0b1000000101100010,
		0b0100000010111011,
	};
	// clang-format on

	std::uint16_t index = 0;
	for(int i = 16 - 1; i >= 0; --i)
	{
		if((sample & masksInversed[i]) != 0)
		{
			index |= masks[i];
			sample ^= matrix[i];
		}
	}

	assert(sample == 0);

	return index;
}

OQMC_HOST_DEVICE constexpr std::uint16_t
sobolPartionIndex(std::uint16_t index, int log2npartition, int partition)
{
	assert(log2npartition >= 0);
	assert(partition >= 0);

	// Method by Keller and Grunschlob, described in 'Parallel Quasi-Monte Carlo
	// Integration by Partitioning Low Discrepancy Sequences'.

	const auto j = partition;
	const auto l = index;
	const auto m = log2npartition;
	const auto n = 1 << m;

	assert(partition < n);

	const auto ln = l * n;
	const auto yl = sobolDimension5(ln);
	const auto sum = (j << (16 - m)) ^ (yl & ~((1 << (16 - m)) - 1));
	const auto kjl = sobolDimension5Inv(sum);
	const auto ijl = ln + kjl;

	return ijl;
}

OQMC_HOST_DEVICE constexpr std::uint32_t sobol(std::uint32_t index,
                                               int dimension)
{
	index = reverseBits32(index);
	index = sobolReversedIndex(index >> 16, dimension);
	index = reverseBits32(index);

	return index;
}

template <int Depth>
OQMC_HOST_DEVICE inline void
partionedScrambledSobol(std::uint32_t index, std::uint32_t seed, int partition,
                        int log2npartition, std::uint32_t sample[Depth])
{
	static_assert(Depth >= 1, "Pattern depth is greater or equal to one.");
	static_assert(Depth <= 4, "Pattern depth is less or equal to four.");

	const auto mask = (1 << log2npartition) - 1;
	partition = shuffle(partition, seed) & mask;

	index = shuffle(index, seed);
	index = sobolPartionIndex(index, log2npartition, partition);

	for(int i = 0; i < Depth; ++i)
	{
		sample[i] = sobol(index, i);
		sample[i] = shuffle(sample[i], rotateBytes(seed, i));
	}
}

} // namespace oqmc
