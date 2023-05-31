// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include <oqmc/reverse.h>

#include <gtest/gtest.h>

#include <cstdint>

namespace
{

TEST(ReverseTest, 32Bit)
{
	// clang-format off
	constexpr std::uint32_t in[5] = {
		0b01010101010101010011001100110011,
		0b11111111000000001111000011110000,
		0b11111111111111110000000011111111,
		0b11111111111111111111111111111111,
		0b00000000000000000000000000000000,
	};

	constexpr std::uint32_t out[5] = {
		0b11001100110011001010101010101010,
		0b00001111000011110000000011111111,
		0b11111111000000001111111111111111,
		0b11111111111111111111111111111111,
		0b00000000000000000000000000000000,
	};
	// clang-format on

	for(int i = 0; i < 5; ++i)
	{
		EXPECT_EQ(in[i], oqmc::reverseBits32(out[i]));
	}
}

TEST(ReverseTest, 16Bit)
{
	// clang-format off
	constexpr std::uint16_t in[5] = {
		0b0101010100110011,
		0b0000000011110000,
		0b1111111100000000,
		0b1111111111111111,
		0b0000000000000000,
	};

	constexpr std::uint16_t out[5] = {
		0b1100110010101010,
		0b0000111100000000,
		0b0000000011111111,
		0b1111111111111111,
		0b0000000000000000,
	};
	// clang-format on

	for(int i = 0; i < 5; ++i)
	{
		EXPECT_EQ(in[i], oqmc::reverseBits16(out[i]));
	}
}

} // namespace
