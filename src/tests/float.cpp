// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include <oqmc/float.h>

#include <gtest/gtest.h>

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

// Some tests are disabled because they are slow, especially in debug builds.
// TODO: enable this option with fuzzy testing in [#88](#88) to reduce compute.
#define ENABLE_EXPENSIVE_TESTS 0

namespace
{

const auto floatOneMinusEpsilon = std::nextafterf(1.0f, 0.0f);

TEST(FloatTest, OneOverTwoPower32)
{
	constexpr auto twoPow32 = 1ull << 32; // 2^32

	EXPECT_EQ(oqmc::floatOneOverTwoPower32, 1.0f / twoPow32);
}

TEST(FloatTest, Minimum)
{
	EXPECT_EQ(oqmc::uintToFloat(0u), 0.0f);
	EXPECT_GT(oqmc::uintToFloat(1u), 0.0f);
	EXPECT_LT(oqmc::uintToFloat(1u), oqmc::uintToFloat(2u));
}

TEST(FloatTest, Maximum)
{
	EXPECT_EQ(oqmc::uintToFloat(UINT32_MAX), floatOneMinusEpsilon);
}

TEST(FloatTest, High)
{
	// Exactly 256 input values [0xffffff00, 0xffffffff] output
	// floatOneMinusEpsilon.
	EXPECT_LT(oqmc::uintToFloat(0xfffffeff), floatOneMinusEpsilon);
	EXPECT_EQ(oqmc::uintToFloat(0xffffff00), floatOneMinusEpsilon);
	EXPECT_EQ(oqmc::uintToFloat(0xffffff01), floatOneMinusEpsilon);
	EXPECT_EQ(oqmc::uintToFloat(0xffffffff), floatOneMinusEpsilon);
}

TEST(FloatTest, Half)
{
	// Exactly 256 input values [0x80000000, 0x800000ff] output 0.5
	EXPECT_LT(oqmc::uintToFloat(0x7fffffff), 0.5f);
	EXPECT_EQ(oqmc::uintToFloat(0x80000000), 0.5f);
	EXPECT_EQ(oqmc::uintToFloat(0x80000001), 0.5f);
	EXPECT_EQ(oqmc::uintToFloat(0x800000ff), 0.5f);
	EXPECT_GT(oqmc::uintToFloat(0x80000100), 0.5f);
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

#if ENABLE_EXPENSIVE_TESTS
TEST(FloatTest, Equidistributed)
{
	// Verify that oqmc::uintToFloat() produces a distribution over floats
	// proportional to their representational density. That is, each output
	// float has the correct number of input integers mapping to it.
	//
	// This is equivalent to requiring that, within each power-of-two interval,
	// all outputs have equal probability.
	//
	// For example, every unique output in [0.5, 1.0) has probability 2^-24
	// and corresponds to exactly 256 inputs.

	// Use 64-bit integer to prevent overflow on loop upper bounds.
	using uint64_t = std::uint64_t;

	// Check each power of two in [0,32) corresponding to
	// input range [min,max), where min=2^P and max=2*min.
	for(int power = 0; power < 32; ++power)
	{
		uint64_t min = 1ull << power;
		uint64_t max = min * 2;

		// For each power of two in the range [0,24) there is exactly one
		// output for each input.
		unsigned repeats = 1;

		// For each power of two in the range [24,31) there are multiple inputs
		// that correspond to each output, with a repeat factor of 2^(P-23).
		if(power >= 24)
		{
			repeats = 1ull << (power - 23);
		}

		// Check each range of inputs that map to the same output
		for(uint64_t input = min; input < max; input += repeats)
		{
			auto previous = oqmc::uintToFloat(input - 1);
			auto current = oqmc::uintToFloat(input);

			// Check output of current range equals the expected value,
			// when calculated with exact arithmetic (double is sufficient).
			auto expected = ldexp(static_cast<double>(input), -32);
			EXPECT_EQ(current, expected);

			// Check output of current range is greater than previous
			EXPECT_LT(previous, current);

			// Check every input in the range has same output
			for(uint64_t repeat = 0; repeat < repeats; ++repeat)
			{
				EXPECT_EQ(oqmc::uintToFloat(input + repeat), current);
			}
		}
	}
}
#endif

TEST(FloatTest, PowersOfTwo)
{
	// All inputs that are powers of two should map to an output
	// that exactly equals the expected power of two.

	for(int i = 0; i < 32; i++)
	{
		std::uint32_t input = 1u << i;
		auto inputFloat = static_cast<float>(input);

		auto expect = inputFloat * oqmc::floatOneOverTwoPower32;
		auto actual = oqmc::uintToFloat(input);
		EXPECT_EQ(actual, expect);
	}
}

float bitsToFloat(std::uint32_t value)
{
	float out;
	std::memcpy(&out, &value, sizeof(std::uint32_t));

	return out;
}

TEST(FloatTest, BitsToFloat)
{
	EXPECT_EQ(bitsToFloat(0u), +0.0f);
	EXPECT_EQ(bitsToFloat(1u << 31), -0.0f);
	EXPECT_EQ(bitsToFloat(1u), std::nextafterf(0.0f, 1.0f));
	EXPECT_EQ(bitsToFloat(0x7F << 23), 1.0f);
}

int countLeadingZeros(std::uint32_t value)
{
	assert(value > 0);

#if defined(_MSC_VER)
	auto index = 0ul;
	_BitScanReverse(&index, value);

	return 31 - index;
#else
	return __builtin_clz(value);
#endif
}

TEST(FloatTest, CountLeadingZeros)
{
	EXPECT_EQ(countLeadingZeros(1u), 31);
	EXPECT_EQ(countLeadingZeros(UINT32_MAX), 0);
	EXPECT_EQ(countLeadingZeros(1u << 31 >> 7 | 1u), 7);
}

#if ENABLE_EXPENSIVE_TESTS
// Reference implementation of method detailed in Section 2.1 of
// 'Quasi-Monte Carlo Algorithms (not only) for Graphics Software'
// by Keller, Wächter and Binder.
float uintToFloatReference(std::uint32_t value)
{
	if(value == 0)
	{
		return 0.0f;
	}

	if(value == 1)
	{
		return bitsToFloat(0x5F << 23);
	}

	const auto clz = countLeadingZeros(value);
	const auto bias = static_cast<std::uint32_t>(127);

	// Shift an extra bit as implicit leading one.
	const auto mantissa = value << (clz + 1);
	const auto exponent = bias - (clz + 1);

	return bitsToFloat(exponent << 23 | mantissa >> 9);
}

TEST(FloatTest, MatchReference)
{
	auto i = 0u;

	while(true)
	{
		EXPECT_EQ(oqmc::uintToFloat(i), uintToFloatReference(i));

		if(i++ == UINT32_MAX)
		{
			break;
		}
	}
}
#endif

} // namespace
