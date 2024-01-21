// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include <oqmc/sampler.h>
#include <oqmc/unused.h>

#include <gtest/gtest.h>

namespace
{

class MockImpl
{
	friend oqmc::SamplerInterface<MockImpl>;
	static constexpr std::size_t cacheSize = 0;
};

using MockSampler = oqmc::SamplerInterface<MockImpl>;

TEST(SamplerTest, CacheSize)
{
	const auto cacheSize = MockSampler::cacheSize;
	ASSERT_EQ(cacheSize, 0);
}

TEST(SamplerTest, DefaultConstructor)
{
	const MockSampler sampler;
	OQMC_MAYBE_UNUSED(sampler);
}

TEST(SamplerTest, Copyable)
{
	const MockSampler samplerA;
	const MockSampler samplerB = samplerA;
	OQMC_MAYBE_UNUSED(samplerB);
}

} // namespace
