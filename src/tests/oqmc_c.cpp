// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include <oqmc/oqmc_c.h>

#if defined(OQMC_CABI_SAMPLER_PMJ)
#include <oqmc/pmj.h>
#elif defined(OQMC_CABI_SAMPLER_SOBOL)
#include <oqmc/sobol.h>
#elif defined(OQMC_CABI_SAMPLER_SOBOLBN)
#include <oqmc/sobolbn.h>
#elif defined(OQMC_CABI_SAMPLER_LATTICE)
#include <oqmc/lattice.h>
#elif defined(OQMC_CABI_SAMPLER_LATTICEBN)
#include <oqmc/latticebn.h>
#else
#include <oqmc/pmjbn.h>
#endif

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace
{

// Match the sampler wrapped by the library, which is chosen by the same
// definition when the library is compiled.
#if defined(OQMC_CABI_SAMPLER_PMJ)
using Sampler = oqmc::PmjSampler;
constexpr auto samplerName = "pmj";
#elif defined(OQMC_CABI_SAMPLER_SOBOL)
using Sampler = oqmc::SobolSampler;
constexpr auto samplerName = "sobol";
#elif defined(OQMC_CABI_SAMPLER_SOBOLBN)
using Sampler = oqmc::SobolBnSampler;
constexpr auto samplerName = "sobolbn";
#elif defined(OQMC_CABI_SAMPLER_LATTICE)
using Sampler = oqmc::LatticeSampler;
constexpr auto samplerName = "lattice";
#elif defined(OQMC_CABI_SAMPLER_LATTICEBN)
using Sampler = oqmc::LatticeBnSampler;
constexpr auto samplerName = "latticebn";
#else
using Sampler = oqmc::PmjBnSampler;
constexpr auto samplerName = "pmjbn";
#endif

constexpr auto pixelX = 2; // 1st prime
constexpr auto pixelY = 3; // 2nd prime

template <int Size>
void expectEqualDrawsOfSize(const Sampler& expected, const oqmc_sampler& actual)
{
	std::uint32_t expectedInt[Size];
	std::uint32_t actualInt[Size];
	float expectedFloat[Size];
	float actualFloat[Size];

	expected.drawSample<Size>(expectedInt);
	expected.drawSample<Size>(expectedFloat);
	oqmc_sampler_draw_sample_u(&actual, Size, actualInt);
	oqmc_sampler_draw_sample_f(&actual, Size, actualFloat);

	for(int i = 0; i < Size; ++i)
	{
		EXPECT_EQ(actualInt[i], expectedInt[i]);
		EXPECT_EQ(actualFloat[i], expectedFloat[i]);
	}

	expected.drawRnd<Size>(expectedInt);
	expected.drawRnd<Size>(expectedFloat);
	oqmc_sampler_draw_rnd_u(&actual, Size, actualInt);
	oqmc_sampler_draw_rnd_f(&actual, Size, actualFloat);

	for(int i = 0; i < Size; ++i)
	{
		EXPECT_EQ(actualInt[i], expectedInt[i]);
		EXPECT_EQ(actualFloat[i], expectedFloat[i]);
	}
}

void expectEqualDraws(const Sampler& expected, const oqmc_sampler& actual)
{
	expectEqualDrawsOfSize<1>(expected, actual);
	expectEqualDrawsOfSize<2>(expected, actual);
	expectEqualDrawsOfSize<3>(expected, actual);
	expectEqualDrawsOfSize<4>(expected, actual);
}

class OqmcCTest : public testing::Test
{
  protected:
	void SetUp() override
	{
		cache.resize(oqmc_sampler_cache_size());
		oqmc_sampler_initialise_cache(cache.data());
	}

	std::vector<std::byte> cache;
};

TEST_F(OqmcCTest, Layout)
{
	EXPECT_TRUE(std::is_trivially_copyable_v<Sampler>);
	EXPECT_LE(sizeof(Sampler), sizeof(oqmc_sampler));
	EXPECT_LE(alignof(Sampler), alignof(oqmc_sampler));
}

TEST_F(OqmcCTest, Name)
{
	EXPECT_STREQ(oqmc_sampler_name(), samplerName);
}

TEST_F(OqmcCTest, CacheSize)
{
	EXPECT_EQ(oqmc_sampler_cache_size(), Sampler::cacheSize);
}

TEST_F(OqmcCTest, InitialiseCache)
{
	std::vector<std::byte> expected(Sampler::cacheSize);
	Sampler::initialiseCache(expected.data());

	EXPECT_EQ(cache, expected);
}

TEST_F(OqmcCTest, Create)
{
	for(int frame = 0; frame < 2; ++frame)
	{
		for(int index = 0; index < 8; ++index)
		{
			const auto expected =
			    Sampler(pixelX, pixelY, frame, index, cache.data());
			const auto actual =
			    oqmc_sampler_create(pixelX, pixelY, frame, index, cache.data());

			expectEqualDraws(expected, actual);
		}
	}
}

TEST_F(OqmcCTest, NewDomain)
{
	const auto expected = Sampler(pixelX, pixelY, 0, 0, cache.data());
	const auto actual = oqmc_sampler_create(pixelX, pixelY, 0, 0, cache.data());

	for(int key = 0; key < 4; ++key)
	{
		expectEqualDraws(expected.newDomain(key),
		                 oqmc_sampler_new_domain(&actual, key));
	}
}

TEST_F(OqmcCTest, NewDomainSplit)
{
	constexpr auto size = 4;

	const auto expected = Sampler(pixelX, pixelY, 0, 0, cache.data());
	const auto actual = oqmc_sampler_create(pixelX, pixelY, 0, 0, cache.data());

	for(int index = 0; index < size; ++index)
	{
		expectEqualDraws(
		    expected.newDomainSplit(0, size, index),
		    oqmc_sampler_new_domain_split(&actual, 0, size, index));
	}
}

TEST_F(OqmcCTest, NewDomainDistrib)
{
	const auto expected = Sampler(pixelX, pixelY, 0, 0, cache.data());
	const auto actual = oqmc_sampler_create(pixelX, pixelY, 0, 0, cache.data());

	for(int index = 0; index < 4; ++index)
	{
		expectEqualDraws(expected.newDomainDistrib(0, index),
		                 oqmc_sampler_new_domain_distrib(&actual, 0, index));
	}
}

TEST_F(OqmcCTest, NewDomainChain)
{
	const auto expected = Sampler(pixelX, pixelY, 0, 0, cache.data());
	const auto actual = oqmc_sampler_create(pixelX, pixelY, 0, 0, cache.data());

	for(int index = 0; index < 4; ++index)
	{
		expectEqualDraws(expected.newDomainChain(0, index),
		                 oqmc_sampler_new_domain_chain(&actual, 0, index));
	}
}

TEST_F(OqmcCTest, DomainTree)
{
	const auto expected = Sampler(pixelX, pixelY, 0, 0, cache.data())
	                          .newDomain(1)
	                          .newDomainSplit(2, 4, 3)
	                          .newDomain(5);

	auto actual = oqmc_sampler_create(pixelX, pixelY, 0, 0, cache.data());
	actual = oqmc_sampler_new_domain(&actual, 1);
	actual = oqmc_sampler_new_domain_split(&actual, 2, 4, 3);
	actual = oqmc_sampler_new_domain(&actual, 5);

	expectEqualDraws(expected, actual);
}

TEST_F(OqmcCTest, Copy)
{
	const auto expected = Sampler(pixelX, pixelY, 0, 0, cache.data());
	const auto original =
	    oqmc_sampler_create(pixelX, pixelY, 0, 0, cache.data());

	const auto copy = original;

	expectEqualDraws(expected, copy);
}

} // namespace
