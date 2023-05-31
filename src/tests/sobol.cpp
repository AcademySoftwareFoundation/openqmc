// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "hypothesis.h"
#include <oqmc/sobol.h>

#include <gtest/gtest.h>

#include <cstdint>

namespace
{

constexpr auto pixelX = 2; // 1st prime
constexpr auto pixelY = 3; // 2nd prime

template <int X, int Y>
struct SamplerV1
{
	SamplerV1() : seed(0)
	{
		cache = new char[oqmc::SobolSampler::cacheSize];
		oqmc::SobolSampler::initialiseCache(cache);
	}

	~SamplerV1()
	{
		delete[] static_cast<char*>(cache);
	}

	void initialise(int seed)
	{
		this->seed = seed;
	}

	void sample(int index, std::uint32_t out[2]) const
	{
		const auto base = oqmc::SobolSampler(pixelX, pixelY, 0, index, cache);
		const auto domain = base.newDomain(seed);

		std::uint32_t rnd[4];
		domain.template drawSample<4>(rnd);

		out[0] = rnd[X];
		out[1] = rnd[Y];
	}

	void* cache;
	int seed;
};

ALL_HYPOTHESIS_TESTS(SobolTest, DrawSampleDims01, (SamplerV1<0, 1>()))
ALL_HYPOTHESIS_TESTS(SobolTest, DrawSampleDims02, (SamplerV1<0, 2>()))
ALL_HYPOTHESIS_TESTS(SobolTest, DrawSampleDims03, (SamplerV1<0, 3>()))
ALL_HYPOTHESIS_TESTS(SobolTest, DrawSampleDims12, (SamplerV1<1, 2>()))
ALL_HYPOTHESIS_TESTS(SobolTest, DrawSampleDims13, (SamplerV1<1, 3>()))
ALL_HYPOTHESIS_TESTS(SobolTest, DrawSampleDims23, (SamplerV1<2, 3>()))

} // namespace
