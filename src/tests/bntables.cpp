// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include <oqmc/bntables.h>
#include <oqmc/encode.h>

#include <gtest/gtest.h>

#include <climits>
#include <cstdint>

namespace
{

// clang-format off
constexpr std::uint32_t keys[] = {
	 0,  1,  2,  3,
	 4,  5,  6,  7,
	 8,  9, 10, 11,
	12, 13, 14, 15,
	 0,  1,  2,  3,
	 4,  5,  6,  7,
	 8,  9, 10, 11,
	12, 13, 14, 15,
	 0,  1,  2,  3,
	 4,  5,  6,  7,
	 8,  9, 10, 11,
	12, 13, 14, 15,
	 0,  1,  2,  3,
	 4,  5,  6,  7,
	 8,  9, 10, 11,
	12, 13, 14, 15,
};

constexpr std::uint32_t ranks[] = {
	 0,  1,  2,  3,
	 4,  5,  6,  7,
	 8,  9, 10, 11,
	12, 13, 14, 15,
	 0,  1,  2,  3,
	 4,  5,  6,  7,
	 8,  9, 10, 11,
	12, 13, 14, 15,
	 0,  1,  2,  3,
	 4,  5,  6,  7,
	 8,  9, 10, 11,
	12, 13, 14, 15,
	 0,  1,  2,  3,
	 4,  5,  6,  7,
	 8,  9, 10, 11,
	12, 13, 14, 15,
};

constexpr auto bits = 2;
constexpr auto resolution = 1 << bits;
constexpr auto xBits = bits;
constexpr auto yBits = bits;
constexpr auto zBits = bits;
constexpr auto zero = 0;
constexpr auto prime = 13; // 7th prime
// clang-format on

TEST(BnTablesTest, ZeroTableValue)
{
	const auto value = oqmc::bntables::tableValue<xBits, yBits, zBits>(
	    zero, zero, keys, ranks);

	EXPECT_EQ(value.key, 0);
	EXPECT_EQ(value.rank, 0);
}

TEST(BnTablesTest, MinimumTableValue)
{
	const auto pixelA =
	    oqmc::encodeBits16<xBits, yBits, zBits>({-INT_MAX, -INT_MAX, -INT_MAX});
	const auto valueA = oqmc::bntables::tableValue<xBits, yBits, zBits>(
	    pixelA, zero, keys, ranks);

	const auto pixelB = oqmc::encodeBits16<xBits, yBits, zBits>(
	    {-INT_MAX % resolution, -INT_MAX % resolution, -INT_MAX % resolution});
	const auto valueB = oqmc::bntables::tableValue<xBits, yBits, zBits>(
	    pixelB, zero, keys, ranks);

	EXPECT_EQ(valueA.key, valueB.key);
	EXPECT_EQ(valueA.rank, valueB.rank);
}

TEST(BnTablesTest, MaximumTableValue)
{
	const auto pixelA =
	    oqmc::encodeBits16<xBits, yBits, zBits>({+INT_MAX, +INT_MAX, +INT_MAX});
	const auto valueA = oqmc::bntables::tableValue<xBits, yBits, zBits>(
	    pixelA, zero, keys, ranks);

	const auto pixelB = oqmc::encodeBits16<xBits, yBits, zBits>(
	    {+INT_MAX % resolution, +INT_MAX % resolution, +INT_MAX % resolution});
	const auto valueB = oqmc::bntables::tableValue<xBits, yBits, zBits>(
	    pixelB, zero, keys, ranks);

	EXPECT_EQ(valueA.key, valueB.key);
	EXPECT_EQ(valueA.rank, valueB.rank);
}

TEST(BnTablesTest, ChangingXTableValue)
{
	for(int x = 0; x < prime; ++x)
	{
		const auto pixelA =
		    oqmc::encodeBits16<xBits, yBits, zBits>({x + 0, 0, 0});
		const auto pixelB =
		    oqmc::encodeBits16<xBits, yBits, zBits>({x + 1, 0, 0});

		const auto valueA = oqmc::bntables::tableValue<xBits, yBits, zBits>(
		    pixelA, zero, keys, ranks);

		const auto valueB = oqmc::bntables::tableValue<xBits, yBits, zBits>(
		    pixelB, zero, keys, ranks);

		EXPECT_NE(valueA.key, valueB.key);
		EXPECT_NE(valueA.rank, valueB.rank);
	}
}

TEST(BnTablesTest, ChangingYTableValue)
{
	for(int y = 0; y < prime; ++y)
	{
		const auto pixelA =
		    oqmc::encodeBits16<xBits, yBits, zBits>({0, y + 0, 0});
		const auto pixelB =
		    oqmc::encodeBits16<xBits, yBits, zBits>({0, y + 1, 0});

		const auto valueA = oqmc::bntables::tableValue<xBits, yBits, zBits>(
		    pixelA, zero, keys, ranks);

		const auto valueB = oqmc::bntables::tableValue<xBits, yBits, zBits>(
		    pixelB, zero, keys, ranks);

		EXPECT_NE(valueA.key, valueB.key);
		EXPECT_NE(valueA.rank, valueB.rank);
	}
}

TEST(BnTablesTest, TilingXTableValue)
{
	for(int x = 0; x < prime; ++x)
	{
		const auto pixel = oqmc::encodeBits16<xBits, yBits, zBits>({x, 0, 0});

		const auto value = oqmc::bntables::tableValue<xBits, yBits, zBits>(
		    pixel, zero, keys, ranks);

		EXPECT_EQ(value.key, x % 4);
		EXPECT_EQ(value.rank, x % 4);
	}

	const auto pixelA =
	    oqmc::encodeBits16<xBits, yBits, zBits>({resolution - 1, 0, 0});
	const auto pixelB = oqmc::encodeBits16<xBits, yBits, zBits>({0 - 1, 0, 0});

	const auto valueA = oqmc::bntables::tableValue<xBits, yBits, zBits>(
	    pixelA, zero, keys, ranks);

	const auto valueB = oqmc::bntables::tableValue<xBits, yBits, zBits>(
	    pixelB, zero, keys, ranks);

	EXPECT_EQ(valueA.key, valueB.key);
	EXPECT_EQ(valueA.rank, valueB.rank);
}

TEST(BnTablesTest, TilingYTableValue)
{
	for(int y = 0; y < prime; ++y)
	{
		const auto pixel = oqmc::encodeBits16<xBits, yBits, zBits>({0, y, 0});

		const auto value = oqmc::bntables::tableValue<xBits, yBits, zBits>(
		    pixel, zero, keys, ranks);

		EXPECT_EQ(value.key, y % 4 * 4);
		EXPECT_EQ(value.rank, y % 4 * 4);
	}

	const auto pixelA =
	    oqmc::encodeBits16<xBits, yBits, zBits>({0, resolution - 1, 0});
	const auto pixelB = oqmc::encodeBits16<xBits, yBits, zBits>({0, 0 - 1, 0});

	const auto valueA = oqmc::bntables::tableValue<xBits, yBits, zBits>(
	    pixelA, zero, keys, ranks);

	const auto valueB = oqmc::bntables::tableValue<xBits, yBits, zBits>(
	    pixelB, zero, keys, ranks);

	EXPECT_EQ(valueA.key, valueB.key);
	EXPECT_EQ(valueA.rank, valueB.rank);
}

TEST(BnTablesTest, TilingZTableValue)
{
	for(int z = 0; z < prime; ++z)
	{
		auto pixel = std::uint16_t{};
		auto value = oqmc::bntables::TableReturnValue{};

		pixel = oqmc::encodeBits16<xBits, yBits, zBits>({0, 0, z});

		value = oqmc::bntables::tableValue<xBits, yBits, zBits>(pixel, zero,
		                                                        keys, ranks);

		EXPECT_EQ(value.key, 0);
		EXPECT_EQ(value.rank, 0);

		pixel = oqmc::encodeBits16<xBits, yBits, zBits>({1, 0, z});

		value = oqmc::bntables::tableValue<xBits, yBits, zBits>(pixel, zero,
		                                                        keys, ranks);

		EXPECT_EQ(value.key, 1);
		EXPECT_EQ(value.rank, 1);

		pixel = oqmc::encodeBits16<xBits, yBits, zBits>({0, 1, z});

		value = oqmc::bntables::tableValue<xBits, yBits, zBits>(pixel, zero,
		                                                        keys, ranks);

		EXPECT_EQ(value.key, 4);
		EXPECT_EQ(value.rank, 4);
	}

	const auto pixelA =
	    oqmc::encodeBits16<xBits, yBits, zBits>({0, resolution - 1, 0});
	const auto pixelB = oqmc::encodeBits16<xBits, yBits, zBits>({0, 0 - 1, 0});

	const auto valueA = oqmc::bntables::tableValue<xBits, yBits, zBits>(
	    pixelA, zero, keys, ranks);

	const auto valueB = oqmc::bntables::tableValue<xBits, yBits, zBits>(
	    pixelB, zero, keys, ranks);

	EXPECT_EQ(valueA.key, valueB.key);
	EXPECT_EQ(valueA.rank, valueB.rank);
}

TEST(BnTablesTest, PixelShiftReverseTableValue)
{
	for(int x = 0; x < prime; ++x)
	{
		for(int y = 0; y < prime; ++y)
		{
			for(int z = 0; z < prime; ++z)
			{
				auto pixel = oqmc::encodeBits16<xBits, yBits, zBits>({x, y, z});
				auto shift = oqmc::encodeBits16<xBits, yBits, zBits>({x, y, z});

				auto valueA = oqmc::bntables::tableValue<xBits, yBits, zBits>(
				    pixel, zero, keys, ranks);

				auto valueB = oqmc::bntables::tableValue<xBits, yBits, zBits>(
				    zero, shift, keys, ranks);

				EXPECT_EQ(valueA.key, valueB.key);
				EXPECT_EQ(valueA.rank, valueB.rank);
			}
		}
	}
}

TEST(BnTablesTest, KeyRankEqualTableValue)
{
	for(int x = 0; x < prime; ++x)
	{
		for(int y = 0; y < prime; ++y)
		{
			for(int z = 0; z < prime; ++z)
			{
				auto pixel = oqmc::encodeBits16<xBits, yBits, zBits>({x, y, z});
				auto shift = oqmc::encodeBits16<xBits, yBits, zBits>({x, y, z});

				auto value = oqmc::bntables::tableValue<xBits, yBits, zBits>(
				    pixel, shift, keys, ranks);

				EXPECT_EQ(value.key, value.rank);
			}
		}
	}
}

} // namespace
