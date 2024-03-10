// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "hypothesis.h"
#include <oqmc/pcg.h>
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

		out[0] = oqmc::pcg::rng(state);
		out[1] = oqmc::pcg::rng(state);
	}

	std::uint32_t state;
};

struct SamplerV2
{
	void initialise(int seed)
	{
		hash = oqmc::pcg::hash(seed);
	}

	void sample(int index, std::uint32_t out[2]) const
	{
		out[0] = oqmc::pcg::hash(hash + index * 2 + 0);
		out[1] = oqmc::pcg::hash(hash + index * 2 + 1);
	}

	std::uint32_t hash;
};

struct SamplerV3
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
		rnd[0] = oqmc::pcg::rngBounded(range, state);
		rnd[1] = oqmc::pcg::rngBounded(range, state);

		out[0] = std::uint32_t(std::uint64_t(rnd[0]) * scalar >> 32);
		out[1] = std::uint32_t(std::uint64_t(rnd[1]) * scalar >> 32);
	}

	std::uint32_t state;
};

ALL_HYPOTHESIS_TESTS(PcgTest, Sequential, (SamplerV1()))
ALL_HYPOTHESIS_TESTS(PcgTest, Parallel, (SamplerV2()))
ALL_HYPOTHESIS_TESTS(PcgTest, Bounded, (SamplerV3()))

constexpr std::array<std::uint32_t, 20> primes{
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
};

constexpr std::array<std::uint32_t, 10> powers{
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512,
};

TEST(PcgTest, StateTransitionChange)
{
	EXPECT_NE(oqmc::pcg::stateTransition(0), 0);

	for(const auto input : primes)
	{
		EXPECT_NE(oqmc::pcg::stateTransition(input), input);
	}
}

TEST(PcgTest, OutputPermutation)
{
	EXPECT_EQ(oqmc::pcg::output(0), 0);

	for(const auto input : primes)
	{
		EXPECT_NE(oqmc::pcg::output(input), input);
	}
}

TEST(PcgTest, StateTransitionOutputNotEqual)
{
	EXPECT_NE(oqmc::pcg::stateTransition(0), oqmc::pcg::output(0));

	for(auto input : primes)
	{
		EXPECT_NE(oqmc::pcg::stateTransition(input), oqmc::pcg::output(input));
	}
}

TEST(PcgTest, InitStateDefault)
{
	EXPECT_EQ(oqmc::pcg::init(), oqmc::pcg::init(0));
}

TEST(PcgTest, InitStateNonEqual)
{
	const auto zeroState = oqmc::pcg::init();

	auto lastState = zeroState;
	for(auto seed : primes)
	{
		const auto primeState = oqmc::pcg::init(seed);

		EXPECT_NE(zeroState, primeState);
		EXPECT_NE(lastState, primeState);

		lastState = primeState;
	}

	lastState = zeroState;
	for(auto seed : powers)
	{
		const auto powerState = oqmc::pcg::init(seed);

		EXPECT_NE(zeroState, powerState);
		EXPECT_NE(lastState, powerState);

		lastState = powerState;
	}
}

TEST(PcgTest, HashNonMutatingState)
{
	for(auto before : primes)
	{
		auto key = before;
		oqmc::pcg::hash(key);

		EXPECT_EQ(before, key);
	}
}

TEST(PcgTest, RngMutatingState)
{
	for(auto before : primes)
	{
		auto state = before;
		oqmc::pcg::rng(state);

		EXPECT_NE(before, state);
	}
}

TEST(PcgTest, HashRngEqual)
{
	for(auto seed : primes)
	{
		auto state = oqmc::pcg::init(seed);

		const auto hash = oqmc::pcg::hash(state);
		const auto rnd = oqmc::pcg::rng(state);

		EXPECT_EQ(hash, rnd);
	}
}

TEST(PcgTest, BoundedRange)
{
	for(auto range : primes)
	{
		auto state = oqmc::pcg::init();

		for(int i = 0; i < 128; ++i)
		{
			const auto rnd = oqmc::pcg::rngBounded(range, state);

			EXPECT_LT(rnd, range);
		}
	}

	for(auto range : powers)
	{
		auto state = oqmc::pcg::init();

		for(int i = 0; i < 128; ++i)
		{
			const auto rnd = oqmc::pcg::rngBounded(range, state);

			EXPECT_LT(rnd, range);
		}
	}
}

TEST(PcgTest, BoundedBeginEnd)
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

			const auto rndA = oqmc::pcg::rngBounded(begin, end, stateA);
			const auto rndB = oqmc::pcg::rngBounded(range, stateB);

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

			const auto rndA = oqmc::pcg::rngBounded(begin, end, stateA);
			const auto rndB = oqmc::pcg::rngBounded(range, stateB);

			EXPECT_GE(rndA, begin);
			EXPECT_LT(rndA, end);
			EXPECT_EQ(rndA - begin, rndB);

			state = stateA;
		}
	}
}

} // namespace
