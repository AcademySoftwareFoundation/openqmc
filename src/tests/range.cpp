// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "hypothesis.h"
#include <oqmc/pcg.h>
#include <oqmc/range.h>
#include <oqmc/unused.h>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

namespace
{

struct SamplerV1
{
	void initialise(int seed)
	{
		state = oqmc::pcg::init(seed);
	}

	void sample(int index, std::uint32_t out[2])
	{
		OQMC_MAYBE_UNUSED(index);

		// Does not divide into UINT32_MAX to test debiasing.
		constexpr auto range = UINT32_MAX / 4 * 3;
		constexpr auto scalar = UINT64_MAX / range;

		std::uint32_t rnd[2];
		rnd[0] = oqmc::uintToRange(oqmc::pcg::rng(state), range);
		rnd[1] = oqmc::uintToRange(oqmc::pcg::rng(state), range);

		out[0] = std::uint32_t(std::uint64_t(rnd[0]) * scalar >> 32);
		out[1] = std::uint32_t(std::uint64_t(rnd[1]) * scalar >> 32);
	}

	std::uint32_t state;
};

ALL_HYPOTHESIS_TESTS(RangeTest, Debiased, (SamplerV1()))

constexpr std::array<std::uint32_t, 20> primes{
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
};

constexpr std::array<std::uint32_t, 10> powers{
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512,
};

TEST(RangeTest, BoundedRange)
{
	for(auto range : primes)
	{
		auto state = oqmc::pcg::init();

		for(int i = 0; i < 128; ++i)
		{
			const auto rnd = oqmc::uintToRange(oqmc::pcg::rng(state), range);

			EXPECT_LT(rnd, range);
		}
	}

	for(auto range : powers)
	{
		auto state = oqmc::pcg::init();

		for(int i = 0; i < 128; ++i)
		{
			const auto rnd = oqmc::uintToRange(oqmc::pcg::rng(state), range);

			EXPECT_LT(rnd, range);
		}
	}
}

TEST(RangeTest, BoundedBeginEnd)
{
	for(auto range : primes)
	{
		auto state = oqmc::pcg::init();

		for(int i = 0; i < 128; ++i)
		{
			auto stateA = state;
			auto stateB = state;

			const auto begin = range;
			const auto end = range * 2;

			const auto rndA =
			    oqmc::uintToRange(oqmc::pcg::rng(stateA), begin, end);
			const auto rndB = oqmc::uintToRange(oqmc::pcg::rng(stateB), range);

			EXPECT_GE(rndA, begin);
			EXPECT_LT(rndA, end);
			EXPECT_EQ(rndA - begin, rndB);

			state = stateA;
		}
	}

	for(auto range : powers)
	{
		auto state = oqmc::pcg::init();

		for(int i = 0; i < 128; ++i)
		{
			auto stateA = state;
			auto stateB = state;

			const auto begin = range;
			const auto end = range * 2;

			const auto rndA =
			    oqmc::uintToRange(oqmc::pcg::rng(stateA), begin, end);
			const auto rndB = oqmc::uintToRange(oqmc::pcg::rng(stateB), range);

			EXPECT_GE(rndA, begin);
			EXPECT_LT(rndA, end);
			EXPECT_EQ(rndA - begin, rndB);

			state = stateA;
		}
	}
}

} // namespace
