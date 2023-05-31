// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include <oqmc/encode.h>

#include <gtest/gtest.h>

namespace
{

TEST(EncodeTest, nonZero)
{
	constexpr auto xBits = 1;
	constexpr auto yBits = 1;
	constexpr auto zBits = 1;

	const auto value = oqmc::encodeBits16<xBits, yBits, zBits>({1, 1, 1});
	const auto key = oqmc::decodeBits16<xBits, yBits, zBits>(value);

	EXPECT_EQ(key.x, 1);
	EXPECT_EQ(key.y, 1);
	EXPECT_EQ(key.z, 1);
}

template <int X, int Y, int Z>
void checkInverse()
{
	constexpr auto mini = 0;
	constexpr auto maxi = 16;
	constexpr auto minj = 16;
	constexpr auto maxj = 32;
	constexpr auto mink = 32;
	constexpr auto maxk = 48;

	for(int i = mini; i < maxi; ++i)
	{
		for(int j = minj; j < maxj; ++j)
		{
			for(int k = mink; k < maxk; ++k)
			{
				const auto value = oqmc::encodeBits16<X, Y, Z>({i, j, k});
				const auto key = oqmc::decodeBits16<X, Y, Z>(value);

				const auto x = i % (1 << X);
				const auto y = j % (1 << Y);
				const auto z = k % (1 << Z);

				ASSERT_EQ(key.x, x);
				ASSERT_EQ(key.y, y);
				ASSERT_EQ(key.z, z);
			}
		}
	}
}

TEST(EncodeTest, invertable)
{
	checkInverse<0, 0, 0>();
	checkInverse<5, 5, 5>();
	checkInverse<1, 2, 3>();
	checkInverse<4, 5, 6>();
}

} // namespace
