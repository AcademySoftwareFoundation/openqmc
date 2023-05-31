// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "hypothesis.h"
#include <oqmc/owen.h>
#include <oqmc/pcg.h>

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
		std::uint32_t samples[2];

		oqmc::shuffledScrambledSobol<1>(index, hash0, &samples[0]);
		oqmc::shuffledScrambledSobol<1>(index, hash1, &samples[1]);

		out[0] = samples[0];
		out[1] = samples[1];
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

} // namespace
