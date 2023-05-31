// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include <oqmc/permute.h>
#include <oqmc/reverse.h>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <vector>

namespace
{

constexpr std::array<std::uint32_t, 20> primes{
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
};

// clang-format off
constexpr std::array<std::uint32_t, 5> values{
	0b01010101010101010011001100110011,
	0b11111111000000001111000011110000,
	0b11111111111111110000000011111111,
	0b11111111111111111111111111111111,
	0b00000000000000000000000000000000,
};
// clang-format on

TEST(PermuteTest, LeftNestedHashing)
{
	constexpr std::uint32_t mask = 0b00000000000000001111111111111111;
	constexpr std::uint32_t flip = 0b00000000000000010000000000000000;

	for(const auto value : values)
	{
		for(const auto prime : primes)
		{
			const auto flipped = value ^ flip;

			const auto v1 = oqmc::laineKarrasPermutation(value, prime);
			const auto v2 = oqmc::laineKarrasPermutation(flipped, prime);

			const auto v1First = v1 & mask;
			const auto v2First = v2 & mask;

			EXPECT_EQ(v1First, v2First);

			const auto v1Second = v1 & ~mask;
			const auto v2Second = v2 & ~mask;

			EXPECT_NE(v1Second, v2Second);
		}
	}
}

TEST(PermuteTest, Reverse)
{
	// clang-format off
	constexpr std::array<std::uint32_t, 3> values{
		0b01010101010101010011001100110011,
		0b11111111000000001111000011110000,
		0b11111111111111110000000011111111,
	};
	// clang-format on

	for(const auto value : values)
	{
		for(const auto prime : primes)
		{
			const auto reversed = oqmc::reverseBits32(value);

			EXPECT_NE(value, reversed);

			const auto v1 = oqmc::laineKarrasPermutation(value, prime);
			const auto v2 = oqmc::laineKarrasPermutation(reversed, prime);

			EXPECT_NE(v1, v2);

			const auto shuffled = oqmc::reverseAndShuffle(value, prime);

			EXPECT_EQ(shuffled, v2);
		}
	}
}

TEST(PermuteTest, FullPermutation)
{
	constexpr auto size = 1 << 4;
	constexpr auto mask = size - 1;

	std::array<bool, size> values;

	for(const auto prime : primes)
	{
		values.fill(false);
		for(int i = 0; i < size; ++i)
		{
			const auto shuffled = oqmc::reverseAndShuffle(i, prime);
			const auto permuted = oqmc::reverseBits32(shuffled);

			ASSERT_EQ(permuted, oqmc::shuffle(i, prime));

			const auto index = permuted & mask;

			auto& value = values[index];

			ASSERT_FALSE(value);

			value = true;
		}

		for(const auto value : values)
		{
			EXPECT_TRUE(value);
		}
	}
}

TEST(PermuteTest, ChangeSeed)
{
	for(const auto value : values)
	{
		std::vector<std::uint32_t> results;

		for(const auto prime : primes)
		{
			const auto key = oqmc::reverseAndShuffle(value, prime);

			for(const auto result : results)
			{
				EXPECT_NE(key, result);
			}

			results.push_back(key);
		}
	}
}

} // namespace
