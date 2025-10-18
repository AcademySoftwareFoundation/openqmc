// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include <oqmc/float.h>

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>

namespace
{

TEST(FloatTest, OneOverUintMax)
{
	EXPECT_EQ(oqmc::floatOneOverUintMax, 1.0f / static_cast<float>(UINT32_MAX));
}

TEST(FloatTest, OneMinusEpsilon)
{
	EXPECT_EQ(oqmc::floatOneMinusEpsilon, std::nextafter(1.0f, 0.0f));
}

TEST(FloatTest, Minimum)
{
	EXPECT_EQ(oqmc::uintToFloat(0u), 0.0f);
	EXPECT_GT(oqmc::uintToFloat(1u), 0.0f);
}

TEST(FloatTest, Maximum)
{
	EXPECT_EQ(oqmc::uintToFloat(UINT32_MAX), oqmc::floatOneMinusEpsilon);
}

TEST(FloatTest, HalfValue)
{
	// Note that due to floating point rounding, this rounds up to 0.5 value.
	EXPECT_EQ(oqmc::uintToFloat(UINT32_MAX / 2), 0.5f);
}

TEST(FloatTest, Monotonic)
{
	constexpr auto numberOfSteps = 8;

	float lastValue = 0.0f;

	for(int i = 0; i < numberOfSteps; ++i)
	{
		const std::uint32_t stepInt = UINT32_MAX / numberOfSteps * (i + 1);
		const float stepFloat = oqmc::uintToFloat(stepInt);

		EXPECT_GT(stepFloat, lastValue);

		lastValue = stepFloat;
	}
}

} // namespace
