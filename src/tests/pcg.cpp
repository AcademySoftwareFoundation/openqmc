// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "hypothesis.h"
#include <oqmc/pcg.h>
#include <oqmc/unused.h>

#include <array>
#include <climits>
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

		constexpr auto min = 0u;
		constexpr auto max = (1u << 30) + (1u << 31);

		float rnd[2];
		rnd[0] = static_cast<float>(oqmc::pcg::rngBounded(min, max, state));
		rnd[1] = static_cast<float>(oqmc::pcg::rngBounded(min, max, state));
		rnd[0] = rnd[0] / static_cast<float>(max);
		rnd[1] = rnd[1] / static_cast<float>(max);

		const auto scalar = static_cast<float>(UINT_MAX);
		out[0] = static_cast<std::uint32_t>(rnd[0] * scalar);
		out[1] = static_cast<std::uint32_t>(rnd[1] * scalar);
	}

	std::uint32_t state;
};

ALL_HYPOTHESIS_TESTS(PcgTest, Sequential, (SamplerV1()))
ALL_HYPOTHESIS_TESTS(PcgTest, Parallel, (SamplerV2()))
ALL_HYPOTHESIS_TESTS(PcgTest, Bounded, (SamplerV3()))

constexpr std::array<std::uint32_t, 20> primes{
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
};

TEST(PcgTest, StateTransitionChange)
{
	for(const auto prime : primes)
	{
		const auto value = oqmc::pcg::stateTransition(prime);

		EXPECT_NE(value, prime);
	}
}

TEST(PcgTest, OutputChange)
{
	for(const auto prime : primes)
	{
		const auto value = oqmc::pcg::output(prime);

		EXPECT_NE(value, prime);
	}
}

TEST(PcgTest, StateTransitionOutputNotEqual)
{
	for(auto prime : primes)
	{
		const auto state = oqmc::pcg::stateTransition(prime);
		const auto output = oqmc::pcg::output(prime);

		EXPECT_NE(state, output);
	}
}

TEST(PcgTest, RngMutatingState)
{
	for(auto prime : primes)
	{
		const auto old = prime;
		oqmc::pcg::rng(prime);

		EXPECT_NE(prime, old);
		EXPECT_EQ(prime, oqmc::pcg::stateTransition(old));
	}
}

TEST(PcgTest, RngStateTransitionOutput)
{
	for(auto prime : primes)
	{
		auto old = prime;
		const auto rng = oqmc::pcg::rng(prime);

		EXPECT_EQ(rng, oqmc::pcg::output(oqmc::pcg::stateTransition(old)));
	}
}

TEST(PcgTest, HashRngEqual)
{
	for(auto prime : primes)
	{
		const auto hash = oqmc::pcg::hash(prime);
		const auto rng = oqmc::pcg::rng(prime);

		EXPECT_EQ(hash, rng);
	}
}

TEST(PcgTest, BoundedSignedUnsigned)
{
	for(auto prime : primes)
	{
		for(int i = 1; i < 32; ++i)
		{
			auto resultA = int();

			{
				const auto begin = static_cast<std::int32_t>(-i);
				const auto end = static_cast<std::int32_t>(+i);

				auto state = prime;

				resultA = oqmc::pcg::rngBounded(begin, end, state) - begin;
			}

			auto resultB = int();

			{
				const auto begin = static_cast<std::uint32_t>(0);
				const auto end = static_cast<std::uint32_t>(i * 2);

				auto state = prime;

				resultB = oqmc::pcg::rngBounded(begin, end, state) - begin;
			}

			EXPECT_EQ(resultA, resultB);
		}
	}
}

TEST(PcgTest, Initialisation)
{
	const auto stateA = oqmc::pcg::init();
	const auto stateB = oqmc::pcg::stateTransition(0);

	EXPECT_EQ(stateA, stateB);

	for(auto prime : primes)
	{
		auto stateA = oqmc::pcg::init(prime);
		auto stateB = oqmc::pcg::init() + prime;

		EXPECT_EQ(stateA, stateB);

		stateB = oqmc::pcg::stateTransition(0) + prime;

		oqmc::pcg::rng(stateA);
		oqmc::pcg::rng(stateB);

		EXPECT_EQ(stateA, stateB);
	}
}

} // namespace
