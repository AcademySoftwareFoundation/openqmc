// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include <oqmc/sampler.h>

#include <gtest/gtest.h>

#include <cstddef>

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
	[[maybe_unused]] const MockSampler sampler;
}

TEST(SamplerTest, Copyable)
{
	const MockSampler samplerA;
	[[maybe_unused]] const MockSampler samplerB = samplerA;
}

} // namespace
