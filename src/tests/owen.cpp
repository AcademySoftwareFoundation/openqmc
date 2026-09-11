// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "hypothesis.h"
#include <oqmc/owen.h>
#include <oqmc/pcg.h>
#include <oqmc/reverse.h>

#include <gtest/gtest.h>

#include <array>
#include <cassert>
#include <cstdint>

namespace
{

struct SamplerV1
{
	void initialise(int seed)
	{
		hash0 = oqmc::pcg::hash(seed * 2 + 0);
		hash1 = oqmc::pcg::hash(seed * 2 + 1);
	}

	void sample(int index, std::uint32_t out[2]) const
	{
		std::uint32_t sample[2];

		oqmc::shuffledScrambledSobol<1>(index, hash0, &sample[0]);
		oqmc::shuffledScrambledSobol<1>(index, hash1, &sample[1]);

		out[0] = sample[0];
		out[1] = sample[1];
	}

	std::uint32_t hash0;
	std::uint32_t hash1;
};

template <int X, int Y>
struct SamplerV2
{
	void initialise(int seed)
	{
		hash = oqmc::pcg::hash(seed);
	}

	void sample(int index, std::uint32_t out[2]) const
	{
		std::uint32_t rnd[4];
		oqmc::shuffledScrambledSobol<4>(index, hash, rnd);

		out[0] = rnd[X];
		out[1] = rnd[Y];
	}

	std::uint32_t hash;
};

// Index of the stratum a sample falls in, for a given number of strata. The
// width is computed in 64 bits so that a resolution of one, and the largest
// representable sample value, both stay within range.
int stratumIndex(std::uint32_t value, int resolution)
{
	assert(resolution > 0);

	constexpr auto twoPow32 = 1ull << 32; // 2^32
	const auto width = twoPow32 / resolution;

	return static_cast<int>(value / width);
}

ALL_HYPOTHESIS_TESTS(OwenTest, SampleIndpendent, (SamplerV1()))
ALL_HYPOTHESIS_TESTS(OwenTest, SampleDims01, (SamplerV2<0, 1>()))
ALL_HYPOTHESIS_TESTS(OwenTest, SampleDims02, (SamplerV2<0, 2>()))
ALL_HYPOTHESIS_TESTS(OwenTest, SampleDims03, (SamplerV2<0, 3>()))
ALL_HYPOTHESIS_TESTS(OwenTest, SampleDims12, (SamplerV2<1, 2>()))
ALL_HYPOTHESIS_TESTS(OwenTest, SampleDims13, (SamplerV2<1, 3>()))
ALL_HYPOTHESIS_TESTS(OwenTest, SampleDims23, (SamplerV2<2, 3>()))

TEST(OwenTest, 02Sequence)
{
	// Dimensions 0 and 1, and dimensions 2 and 3, each form a base-2
	// (0, 2)-sequence. The second pair follows from the first through the
	// pair relation of eq (13) (Ahmed et al. 2025).
	constexpr auto m = 16;
	constexpr auto n = 1 << m;

	std::array<bool, n> strata;
	for(int dim = 0; dim < 4; dim += 2)
	{
		ASSERT_LT(dim + 1, 4);

		for(int i = 0; i < m + 1; ++i)
		{
			const int xResolution = 1 << i;
			const int yResolution = 1 << (m - i);

			ASSERT_EQ(xResolution * yResolution, n);

			strata.fill(false);
			for(int index = 0; index < n; ++index)
			{
				std::uint32_t out[4];
				oqmc::shuffledScrambledSobol<4>(index, oqmc::pcg::hash(0), out);

				const int x = stratumIndex(out[dim + 0], xResolution);
				const int y = stratumIndex(out[dim + 1], yResolution);

				const int coordinate = x + y * xResolution;
				auto& stratum = strata[coordinate];

				ASSERT_FALSE(stratum);

				stratum = true;
			}

			for(auto stratum : strata)
			{
				EXPECT_TRUE(stratum);
			}
		}
	}
}

TEST(OwenTest, 02SequenceAllPairs)
{
	// Every pair of dimensions is a base-4 (0, 2)-sequence (Ahmed et al.
	// 2025): each 4^a x 4^b split of the first 4^m samples has exactly one
	// sample per stratum. Taking m to the full precision of the index covers
	// every column of the generator matrices.
	constexpr auto m = 8;
	constexpr auto n = 1 << (2 * m);

	std::array<bool, n> strata;
	for(int dimA = 0; dimA < 4; ++dimA)
	{
		for(int dimB = dimA + 1; dimB < 4; ++dimB)
		{
			for(int i = 0; i < m + 1; ++i)
			{
				const int xResolution = 1 << (2 * i);
				const int yResolution = 1 << (2 * (m - i));

				ASSERT_EQ(xResolution * yResolution, n);

				strata.fill(false);
				for(int index = 0; index < n; ++index)
				{
					std::uint32_t out[4];
					oqmc::shuffledScrambledSobol<4>(index, oqmc::pcg::hash(0),
					                                out);

					const int x = stratumIndex(out[dimA], xResolution);
					const int y = stratumIndex(out[dimB], yResolution);

					const int coordinate = x + y * xResolution;
					auto& stratum = strata[coordinate];

					ASSERT_FALSE(stratum);

					stratum = true;
				}

				for(auto stratum : strata)
				{
					EXPECT_TRUE(stratum);
				}
			}
		}
	}
}

TEST(OwenTest, 04Sequence)
{
	// Dimensions 0-3 form a base-4 (0, 4)-sequence (Ahmed et al. 2025): each
	// 4^a x 4^b x 4^c x 4^d split of the first 4^m samples has exactly one
	// sample per stratum.
	constexpr auto m = 8;
	constexpr auto n = 1 << (2 * m);

	std::array<bool, n> strata;
	for(int a = 0; a <= m; ++a)
	{
		for(int b = 0; b <= m - a; ++b)
		{
			for(int c = 0; c <= m - a - b; ++c)
			{
				const int exponents[4] = {a, b, c, m - a - b - c};

				strata.fill(false);
				for(int index = 0; index < n; ++index)
				{
					std::uint32_t out[4];
					oqmc::shuffledScrambledSobol<4>(index, oqmc::pcg::hash(0),
					                                out);

					int coordinate = 0;
					for(int dim = 0; dim < 4; ++dim)
					{
						const int resolution = 1 << (2 * exponents[dim]);
						const int digit = stratumIndex(out[dim], resolution);

						coordinate = coordinate * resolution + digit;
					}

					auto& stratum = strata[coordinate];

					ASSERT_FALSE(stratum);

					stratum = true;
				}

				for(auto stratum : strata)
				{
					EXPECT_TRUE(stratum);
				}
			}
		}
	}
}

TEST(OwenTest, ShirleyRemapping)
{
	constexpr auto numStratum = 8;
	constexpr auto numSamples = numStratum * numStratum;

	std::array<bool, numStratum> strata;
	for(int i = 0; i < numStratum; ++i)
	{
		constexpr auto width = UINT32_MAX / numStratum;

		strata.fill(false);
		for(int index = 0; index < numSamples; ++index)
		{
			std::uint32_t out[2];
			oqmc::shuffledScrambledSobol<2>(index, oqmc::pcg::hash(0), out);

			const int x = out[0] / width;
			const int y = out[1] / width;

			if(x != i)
			{
				continue;
			}

			auto& stratum = strata[y];

			ASSERT_FALSE(stratum);

			stratum = true;
		}

		for(auto stratum : strata)
		{
			EXPECT_TRUE(stratum);
		}
	}
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
	0b1110011110011110,
	0b0111100111100111,
	0b0011000100100011,
	0b0001001000110001,
	0b0000111000001001,
	0b0000011100001110,
	0b0000001100000010,
	0b0000000100000011,
	0b0000000011100111,
	0b0000000001111001,
	0b0000000000110001,
	0b0000000000010010,
	0b0000000000001110,
	0b0000000000000111,
	0b0000000000000011,
	0b0000000000000001,
	},

	{
	0b1001111001111001,
	0b0111100111100111,
	0b0010001100010010,
	0b0001001000110001,
	0b0000100100000111,
	0b0000011100001110,
	0b0000001000000001,
	0b0000000100000011,
	0b0000000010011110,
	0b0000000001111001,
	0b0000000000100011,
	0b0000000000010010,
	0b0000000000001001,
	0b0000000000000111,
	0b0000000000000010,
	0b0000000000000001,
	},
};
// clang-format on

// Reference code multiplying the SZ generator matrices directly, produced
// by the matrices cli tool. Verifies the Ahmed 2024 closed form programs
// and the eq (13) evaluation of dimension 3 (PR #97).
std::uint16_t szReference(std::uint16_t index, int dimension)
{
	std::uint16_t sample = 0;
	for(int i = 0; i < 16; ++i)
	{
		if((index & masks[i]) != 0)
		{
			sample ^= directions[dimension][i];
		}
	}

	return sample;
}

TEST(OwenTest, SzReversedBasis)
{
	for(int dimension = 0; dimension < 4; ++dimension)
	{
		for(std::uint32_t index = 0; index < (1u << 16); ++index)
		{
			const auto reversed = static_cast<std::uint16_t>(index);

			EXPECT_EQ(oqmc::szReversedBasis(reversed, dimension),
			          szReference(reversed, dimension));
		}
	}
}

TEST(OwenTest, SzToReversed)
{
	for(int dimension = 0; dimension < 4; ++dimension)
	{
		for(std::uint32_t index = 0; index < (1u << 16); ++index)
		{
			const auto value = static_cast<std::uint16_t>(index);

			EXPECT_EQ(oqmc::szToReversed(value, dimension),
			          szReference(oqmc::reverseBits16(value), dimension));
		}
	}
}

TEST(OwenTest, SzPairRelation)
{
	// The host path of oqmc::shuffledScrambledSobol takes dimension 3 from
	// dimension 2, P * SZ[2] == SZ[3] (Ahmed et al. 2025, eq (13)).
	for(std::uint32_t index = 0; index < (1u << 16); ++index)
	{
		const auto value = static_cast<std::uint16_t>(index);

		EXPECT_EQ(oqmc::szToReversed(oqmc::szToReversed(value, 2), 1),
		          oqmc::szToReversed(value, 3));
	}
}

} // namespace
