// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "hypothesis.h"
#include <oqmc/state.h>
#include <oqmc/unused.h>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <vector>

namespace
{

constexpr auto frame = 2;        // 1st prime
constexpr auto index = 3;        // 2nd prime
constexpr auto pixelX = 5;       // 3rd prime
constexpr auto pixelY = 7;       // 4th prime
constexpr auto lowValue = 11;    // 5th prime
constexpr auto highValue = 3571; // 500th prime

constexpr std::array<int, 20> primes{
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
};

const oqmc::State64Bit defaultState(pixelX, pixelY, frame, index);

TEST(StateTest, AlterPixelFrameLow)
{
	oqmc::State64Bit lastI(0, 0, 0, index);
	oqmc::State64Bit lastDecorrelatedI = lastI.pixelDecorrelate();

	for(int i = 1; i < lowValue; ++i)
	{
		oqmc::State64Bit lastJ(i, 0, 0, index);
		oqmc::State64Bit lastDecorrelatedJ = lastJ.pixelDecorrelate();

		EXPECT_EQ(lastJ.patternId, lastI.patternId);
		EXPECT_EQ(lastJ.sampleId, lastI.sampleId);
		EXPECT_NE(lastJ.pixelId, lastI.pixelId);

		EXPECT_NE(lastDecorrelatedJ.patternId, lastDecorrelatedI.patternId);

		lastI = lastJ;
		lastDecorrelatedI = lastDecorrelatedJ;

		for(int j = 1; j < lowValue; ++j)
		{
			oqmc::State64Bit lastK(i, j, 0, index);
			oqmc::State64Bit lastDecorrelatedK = lastK.pixelDecorrelate();

			EXPECT_EQ(lastK.patternId, lastJ.patternId);
			EXPECT_EQ(lastK.sampleId, lastJ.sampleId);
			EXPECT_NE(lastK.pixelId, lastJ.pixelId);

			EXPECT_NE(lastDecorrelatedK.patternId, lastDecorrelatedJ.patternId);

			lastJ = lastK;
			lastDecorrelatedJ = lastDecorrelatedK;

			for(int k = 1; k < lowValue; ++k)
			{
				oqmc::State64Bit lastN(i, j, k, index);
				oqmc::State64Bit lastDecorrelatedN = lastN.pixelDecorrelate();

				EXPECT_EQ(lastN.patternId, lastK.patternId);
				EXPECT_EQ(lastN.sampleId, lastK.sampleId);
				EXPECT_NE(lastN.pixelId, lastK.pixelId);

				EXPECT_NE(lastDecorrelatedN.patternId,
				          lastDecorrelatedK.patternId);

				lastK = lastN;
				lastDecorrelatedK = lastDecorrelatedN;
			}
		}
	}
}

TEST(StateTest, AlterPixelFrameHigh)
{
	oqmc::State64Bit lastI(highValue, highValue, highValue, index);
	oqmc::State64Bit lastDecorrelatedI = lastI.pixelDecorrelate();

	for(int i = highValue + 1; i < highValue + lowValue; ++i)
	{
		oqmc::State64Bit lastJ(i, highValue, highValue, index);
		oqmc::State64Bit lastDecorrelatedJ = lastJ.pixelDecorrelate();

		EXPECT_EQ(lastJ.patternId, lastI.patternId);
		EXPECT_EQ(lastJ.sampleId, lastI.sampleId);
		EXPECT_NE(lastJ.pixelId, lastI.pixelId);

		EXPECT_NE(lastDecorrelatedJ.patternId, lastDecorrelatedI.patternId);

		lastI = lastJ;
		lastDecorrelatedI = lastDecorrelatedJ;

		for(int j = highValue + 1; j < highValue + lowValue; ++j)
		{
			oqmc::State64Bit lastK(i, j, highValue, index);
			oqmc::State64Bit lastDecorrelatedK = lastK.pixelDecorrelate();

			EXPECT_EQ(lastK.patternId, lastJ.patternId);
			EXPECT_EQ(lastK.sampleId, lastJ.sampleId);
			EXPECT_NE(lastK.pixelId, lastJ.pixelId);

			EXPECT_NE(lastDecorrelatedK.patternId, lastDecorrelatedJ.patternId);

			lastJ = lastK;
			lastDecorrelatedJ = lastDecorrelatedK;

			for(int k = highValue + 1; k < highValue + lowValue; ++k)
			{
				oqmc::State64Bit lastN(i, j, k, index);
				oqmc::State64Bit lastDecorrelatedN = lastN.pixelDecorrelate();

				EXPECT_EQ(lastN.patternId, lastK.patternId);
				EXPECT_EQ(lastN.sampleId, lastK.sampleId);
				EXPECT_NE(lastN.pixelId, lastK.pixelId);

				EXPECT_NE(lastDecorrelatedN.patternId,
				          lastDecorrelatedK.patternId);

				lastK = lastN;
				lastDecorrelatedK = lastDecorrelatedN;
			}
		}
	}
}

TEST(StateTest, AlterSample)
{
	for(const auto prime : primes)
	{
		const oqmc::State64Bit state(pixelX, pixelY, frame, prime);
		EXPECT_EQ(state.patternId, defaultState.patternId);
		EXPECT_EQ(state.pixelId, defaultState.pixelId);

		if(prime == index)
		{
			EXPECT_EQ(state.sampleId, defaultState.sampleId);
		}
		else
		{
			EXPECT_NE(state.sampleId, defaultState.sampleId);
		}
	}

	{
		constexpr auto sizeMinusZero = oqmc::State64Bit::maxIndexSize - 0;
		constexpr auto sizeMinusOne = oqmc::State64Bit::maxIndexSize - 1;

		const oqmc::State64Bit stateA(pixelX, pixelY, frame, sizeMinusZero);
		const oqmc::State64Bit stateB(pixelX, pixelY, frame, sizeMinusOne);

		EXPECT_EQ(stateA.sampleId, 0);
		EXPECT_EQ(stateB.sampleId, sizeMinusOne);
		EXPECT_NE(stateA.patternId, stateB.patternId);
	}
}

TEST(StateTest, NewDomain)
{
	std::vector<std::uint32_t> resultsState;
	std::vector<std::uint32_t> resultsDistr;
	std::vector<std::uint32_t> resultsSplit;

	for(const auto prime : primes)
	{
		const auto state = defaultState.newDomain(prime);
		const auto distr = defaultState.newDomainDistrib(prime, 0);
		const auto split = defaultState.newDomainSplit(prime, lowValue, 0);

		EXPECT_NE(state.patternId, defaultState.patternId);
		EXPECT_NE(distr.patternId, defaultState.patternId);
		EXPECT_NE(split.patternId, defaultState.patternId);

		EXPECT_EQ(state.sampleId, defaultState.sampleId);
		EXPECT_EQ(distr.sampleId, 0);
		EXPECT_GE(split.sampleId, defaultState.sampleId);

		EXPECT_EQ(state.pixelId, defaultState.pixelId);
		EXPECT_EQ(distr.pixelId, defaultState.pixelId);
		EXPECT_EQ(split.pixelId, defaultState.pixelId);

		EXPECT_NE(state.patternId, distr.patternId);
		EXPECT_NE(state.patternId, split.patternId);

		for(const auto result : resultsState)
		{
			ASSERT_NE(state.patternId, result);
		}

		for(const auto result : resultsDistr)
		{
			ASSERT_NE(distr.patternId, result);
		}

		for(const auto result : resultsSplit)
		{
			ASSERT_NE(split.patternId, result);
		}

		resultsState.push_back(state.patternId);
		resultsDistr.push_back(distr.patternId);
		resultsSplit.push_back(split.patternId);
	}
}

TEST(StateTest, NextDomainIndex)
{
	for(const auto prime : primes)
	{
		const auto distr = defaultState.newDomainDistrib(prime, 0);
		const auto split = defaultState.newDomainSplit(prime, lowValue, 0);

		for(int i = 0; i < lowValue; ++i)
		{
			const auto next = defaultState.newDomainDistrib(prime, i);
			EXPECT_EQ(next.sampleId, distr.sampleId + i);
		}

		for(int i = 0; i < lowValue; ++i)
		{
			const auto next = defaultState.newDomainSplit(prime, lowValue, i);
			EXPECT_EQ(next.sampleId, split.sampleId + i);
		}
	}
}

TEST(StateTest, ComputeIndexKey)
{
	constexpr auto index = 1234 << 16 | 5678;
	EXPECT_EQ(oqmc::computeIndexKey(index), 1234);
}

TEST(StateTest, ComputeIndexId)
{
	constexpr auto index = 1234 << 16 | 5678;
	EXPECT_EQ(oqmc::computeIndexId(index), 5678);
}

template <int X, int Y>
struct SamplerV1
{
	void initialise(int seed)
	{
		this->seed = seed;
	}

	void sample(int index, std::uint32_t out[2]) const
	{
		const auto base = oqmc::State64Bit(pixelX, pixelY, 0, index);
		const auto domainA = base.newDomain(seed);
		const auto domainB = domainA.newDomain(0);

		std::uint32_t rnd[4];
		domainA.template drawRnd<2>(&rnd[0]);
		domainB.template drawRnd<2>(&rnd[2]);

		out[0] = rnd[X];
		out[1] = rnd[Y];
	}

	int seed;
};

ALL_HYPOTHESIS_TESTS(StateTest, DrawRndDims01, (SamplerV1<0, 1>()))
ALL_HYPOTHESIS_TESTS(StateTest, DrawRndDims02, (SamplerV1<0, 2>()))
ALL_HYPOTHESIS_TESTS(StateTest, DrawRndDims03, (SamplerV1<0, 3>()))
ALL_HYPOTHESIS_TESTS(StateTest, DrawRndDims12, (SamplerV1<1, 2>()))
ALL_HYPOTHESIS_TESTS(StateTest, DrawRndDims13, (SamplerV1<1, 3>()))
ALL_HYPOTHESIS_TESTS(StateTest, DrawRndDims23, (SamplerV1<2, 3>()))

} // namespace
