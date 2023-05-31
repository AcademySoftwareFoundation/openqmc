// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/**
 * @file
 * @details An efficient implementation of progressive multi-jittered (0,2)
 * sequences. This can be used to construct higher level sampler types. The
 * method is based on 'Stochastic Generation of (t, s) Sample Sequences'
 * by Andrew Helmer, et al. As progressive multi-jittered (0,2) XOR tables
 * only produce the first pair of the four dimensions, the second pair is a
 * randomisation of the first.
 */

#pragma once

#include "lookup.h"
#include "pcg.h"
#include "unused.h"

#include <cassert>
#include <cstdint>

namespace oqmc
{

/**
 * @brief Initialise a table with a progressive mult-jittered (0,2) sequence.
 * @details Given a data array and size, compute the corresponding progressive
 * multi-jittered (0,2) sequence value for each element of the array. Each
 * element in the array is a 4 dimensional sample. Number of samples must be
 * larger than zero and no more than 2^16.
 *
 * @param [in] nsamples Size of the table array.
 * @param [out] table Output array of 4 dimensional samples.
 */
inline void stochasticPmjInit(int nsamples, std::uint32_t table[][4])
{
	constexpr auto maxIndexSize = 0x10000; // 2^16 index upper limit.
	OQMC_MAYBE_UNUSED(maxIndexSize);

	assert(nsamples >= 1);
	assert(nsamples <= maxIndexSize);

	// clang-format off
	constexpr std::uint16_t pmjXors[2][16] = {
		{
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000010,
		0b0000000000000110,
		0b0000000000000110,
		0b0000000000001110,
		0b0000000000110110,
		0b0000000001001110,
		0b0000000000010110,
		0b0000000000101110,
		0b0000001001110110,
		0b0000011011001110,
		0b0000011100010110,
		0b0000110000101110,
		0b0011000001110110,
		0b0100000011001110,
		},

		{
		0b0000000000000000,
		0b0000000000000001,
		0b0000000000000011,
		0b0000000000000011,
		0b0000000000000111,
		0b0000000000011011,
		0b0000000000100111,
		0b0000000000001011,
		0b0000000000010111,
		0b0000000100111011,
		0b0000001101100111,
		0b0000001110001011,
		0b0000011000010111,
		0b0001100000111011,
		0b0010000001100111,
		0b0000000010001011,
		},
	};
	// clang-format on

	const auto buffer = new std::uint32_t[nsamples][2];

	auto state = pcg::init();

	for(int k = 0; k < 2; ++k)
	{
		buffer[0][k] = pcg::rng(state);
	}

	for(int prevLen = 1, logN = 0; prevLen < nsamples; prevLen *= 2, ++logN)
	{
		for(int i1 = 0, i2 = prevLen; i1 < prevLen && i2 < nsamples; ++i1, ++i2)
		{
			for(int k = 0; k < 2; ++k)
			{
				const auto swapBit = 0x80000000u >> logN;
				const auto bitMask = swapBit - 1;

				const auto j = i1 ^ pmjXors[k][logN];

				const auto prevStratum = buffer[j][k] & ~bitMask;
				const auto nextStratum = prevStratum ^ swapBit;

				buffer[i2][k] = nextStratum | (pcg::rng(state) & bitMask);
			}
		}
	}

	for(int i = 0; i < nsamples; ++i)
	{
		shuffledScrambledLookup<2, 2>(i, pcg::hash(0), buffer, &table[i][0]);
		shuffledScrambledLookup<2, 2>(i, pcg::hash(1), buffer, &table[i][2]);
	}

	delete[] buffer;
}

} // namespace oqmc
