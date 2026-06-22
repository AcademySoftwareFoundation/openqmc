// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "hypothesis.h"
#include <oqmc/owen.h>
#include <oqmc/pcg.h>
#include <oqmc/reverse.h>

#include <gtest/gtest.h>

#include <array>
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

ALL_HYPOTHESIS_TESTS(OwenTest, SampleIndpendent, (SamplerV1()))
ALL_HYPOTHESIS_TESTS(OwenTest, SampleDims01, (SamplerV2<0, 1>()))
ALL_HYPOTHESIS_TESTS(OwenTest, SampleDims02, (SamplerV2<0, 2>()))
ALL_HYPOTHESIS_TESTS(OwenTest, SampleDims03, (SamplerV2<0, 3>()))
ALL_HYPOTHESIS_TESTS(OwenTest, SampleDims12, (SamplerV2<1, 2>()))
ALL_HYPOTHESIS_TESTS(OwenTest, SampleDims13, (SamplerV2<1, 3>()))
ALL_HYPOTHESIS_TESTS(OwenTest, SampleDims23, (SamplerV2<2, 3>()))

TEST(OwenTest, 02Sequence)
{
	constexpr auto m = 8;
	constexpr auto n = 1 << m;

	std::array<bool, n> strata;
	for(int i = 0; i < m + 1; ++i)
	{
		const int xResolution = 1 << i;
		const int yResolution = 1 << (m - i);

		ASSERT_EQ(xResolution * yResolution, n);

		const std::uint32_t xWidth = UINT32_MAX / xResolution;
		const std::uint32_t yWidth = UINT32_MAX / yResolution;

		strata.fill(false);
		for(int index = 0; index < n; ++index)
		{
			std::uint32_t out[2];
			oqmc::shuffledScrambledSobol<2>(index, oqmc::pcg::hash(0), out);

			const int x = out[0] / xWidth;
			const int y = out[1] / yWidth;

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

TEST(OwenTest, SobolReversedIndex)
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

	constexpr std::uint16_t directions[4][16] = {
		{0x8000, 0x4000, 0x2000, 0x1000, 0x0800, 0x0400, 0x0200, 0x0100,
		 0x0080, 0x0040, 0x0020, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001},
		{0xffff, 0x5555, 0x3333, 0x1111, 0x0f0f, 0x0505, 0x0303, 0x0101,
		 0x00ff, 0x0055, 0x0033, 0x0011, 0x000f, 0x0005, 0x0003, 0x0001},
		{0xaa09, 0x7706, 0x3903, 0x1601, 0x09aa, 0x0677, 0x0339, 0x0116,
		 0x00a3, 0x0071, 0x003a, 0x0017, 0x0009, 0x0006, 0x0003, 0x0001},
		{0xa0c3, 0x4041, 0x302d, 0x101e, 0x0b67, 0x079a, 0x02a4, 0x011b,
		 0x00c9, 0x0045, 0x002e, 0x001f, 0x000a, 0x0004, 0x0003, 0x0001},
	};
	// clang-format on

	// Reference code from the classic scalar implementation. Verifies that
	// Ahmed 2024 closed form output matches (PR #97).
	const auto reference = [&](std::uint16_t index, int dimension) {
		if(dimension == 0)
		{
			return oqmc::reverseBits16(index);
		}

		const auto matrix = directions[dimension];

		std::uint16_t sample = 0;
		for(int i = 0; i < 16; ++i)
		{
			if((index & masks[i]) != 0)
			{
				sample ^= matrix[i];
			}
		}

		return sample;
	};

	for(int dimension = 0; dimension < 4; ++dimension)
	{
		for(std::uint32_t index = 0; index < (1u << 16); ++index)
		{
			const auto value = static_cast<std::uint16_t>(index);

			EXPECT_EQ(oqmc::sobolReversedIndex(value, dimension),
			          reference(value, dimension));
		}
	}
}

} // namespace
