// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include <oqmc/float.h>

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>

namespace
{

TEST(FloatTest, BitsToFloat)
{
	EXPECT_EQ(oqmc::bitsToFloat(0u), +0.0f);
	EXPECT_EQ(oqmc::bitsToFloat(1u << 31), -0.0f);
	EXPECT_EQ(oqmc::bitsToFloat(1u), std::nextafterf(0.0f, 1.0f));
	EXPECT_EQ(oqmc::bitsToFloat(0x7F << 23), 1.0f);
}

TEST(FloatTest, CountLeadingZeros)
{
	EXPECT_EQ(oqmc::countLeadingZeros(1u), 31);
	EXPECT_EQ(oqmc::countLeadingZeros(UINT32_MAX), 0);
	EXPECT_EQ(oqmc::countLeadingZeros(1u << 31 >> 7 | 1u), 7);
}

TEST(FloatTest, Minimum)
{
	EXPECT_EQ(oqmc::uintToFloat(0u), 0.0f);
	EXPECT_GT(oqmc::uintToFloat(1u), 0.0f);
	EXPECT_LT(oqmc::uintToFloat(1u), oqmc::uintToFloat(2u));
}

TEST(FloatTest, Maximum)
{
	EXPECT_EQ(oqmc::uintToFloat(UINT32_MAX), std::nextafterf(1.0f, 0.0f));
}

TEST(FloatTest, HalfValue)
{
	EXPECT_EQ(oqmc::uintToFloat(UINT32_MAX / 2 + 1), 0.5f);
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
