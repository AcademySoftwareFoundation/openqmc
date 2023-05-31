// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "hypothesis.h"
#include <oqmc/float.h>
#include <oqmc/pcg.h>
#include <oqmc/rank1.h>

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

		oqmc::shuffledRotatedLattice<1>(index, hash0, &samples[0]);
		oqmc::shuffledRotatedLattice<1>(index, hash1, &samples[1]);

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
		oqmc::shuffledRotatedLattice<4>(index, hash, rnd);

		out[0] = rnd[X];
		out[1] = rnd[Y];
	}

	std::uint32_t hash;
};

ALL_HYPOTHESIS_TESTS(Rank1Test, SampleIndpendent, (SamplerV1()))
ALL_HYPOTHESIS_TESTS(Rank1Test, SampleDims01, (SamplerV2<0, 1>()))
ALL_HYPOTHESIS_TESTS(Rank1Test, SampleDims02, (SamplerV2<0, 2>()))
ALL_HYPOTHESIS_TESTS(Rank1Test, SampleDims03, (SamplerV2<0, 3>()))
ALL_HYPOTHESIS_TESTS(Rank1Test, SampleDims12, (SamplerV2<1, 2>()))
ALL_HYPOTHESIS_TESTS(Rank1Test, SampleDims13, (SamplerV2<1, 3>()))
ALL_HYPOTHESIS_TESTS(Rank1Test, SampleDims23, (SamplerV2<2, 3>()))

constexpr std::array<int, 20> primes{
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
};

constexpr auto oneEighthInt = 1 << 29;
constexpr auto oneEighthFloat = 1.f / 8;

TEST(Rank1Test, Rotate)
{
	for(const auto prime : primes)
	{
		const auto stepUint32 = static_cast<std::uint32_t>(oneEighthInt);
		const auto stepFloat = oneEighthFloat;

		const auto multUint32 = stepUint32 * prime;
		const auto multFloat = stepFloat * prime;

		const auto valueUint32 = oqmc::uintToFloat(multUint32);
		const auto valueFloat = multFloat - static_cast<int>(multFloat);

		EXPECT_FLOAT_EQ(valueUint32, valueFloat);
	}
}

TEST(Rank1Test, LatticeReversedIndexIndices)
{
	std::uint32_t last[] = {
	    oqmc::latticeReversedIndex(0, 0),
	    oqmc::latticeReversedIndex(0, 1),
	    oqmc::latticeReversedIndex(0, 2),
	    oqmc::latticeReversedIndex(0, 3),
	};

	for(const auto prime : primes)
	{
		std::uint32_t next[] = {
		    oqmc::latticeReversedIndex(prime, 0),
		    oqmc::latticeReversedIndex(prime, 1),
		    oqmc::latticeReversedIndex(prime, 2),
		    oqmc::latticeReversedIndex(prime, 3),
		};

		for(int i = 0; i < 4; ++i)
		{
			EXPECT_NE(last[i], next[i]);
		}

		for(int i = 0; i < 4; ++i)
		{
			last[i] = next[i];
		}
	}
}

TEST(Rank1Test, LatticeReversedIndexDimensions)
{
	for(const auto prime : primes)
	{
		std::uint32_t value[] = {
		    oqmc::latticeReversedIndex(prime, 0),
		    oqmc::latticeReversedIndex(prime, 1),
		    oqmc::latticeReversedIndex(prime, 2),
		    oqmc::latticeReversedIndex(prime, 3),
		};

		for(int i = 0; i < 4; ++i)
		{
			for(int j = 0; j < 4; ++j)
			{
				if(i == j)
				{
					continue;
				}

				EXPECT_NE(value[i], value[j]);
			}
		}
	}
}

} // namespace
