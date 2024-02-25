// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#pragma once

#include "../shapes.h"
#include <oqmc/float.h>

#include <gtest/gtest.h>
#include <hypothesis/hypothesis.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

constexpr auto defaultResolution = 31;        // 11th prime
constexpr auto defaultNumSamplesLow = 61;     // 18th prime
constexpr auto defaultNumSamplesHigh = 48611; // 5000th prime
constexpr auto defaultNumSeeds = 4;
constexpr auto defaultNumHeavisides = 4;
constexpr auto defaultSignificanceLevel = 0.05f;

template <typename UnitType>
class RunningStats
{
	int n;
	UnitType m1;
	UnitType m2;

  public:
	RunningStats() : n(), m1(), m2()
	{
	}

	void push(UnitType x)
	{
		const auto n1 = n;
		n++;

		const auto delta = x - m1;
		const auto deltaOverN = delta / n;

		m1 += deltaOverN;
		m2 += delta * deltaOverN * n1;
	}

	int numDataValues() const
	{
		return n;
	}

	UnitType mean() const
	{
		return m1;
	}

	UnitType variance() const
	{
		return m2 / (n - 1);
	}

	UnitType standardDeviation() const
	{
		return std::sqrt(variance());
	}

	friend RunningStats operator+(RunningStats a, RunningStats b)
	{
		RunningStats combined;
		combined.n = a.n + b.n;

		const auto delta = b.m1 - a.m1;
		const auto delta2 = delta * delta;

		combined.m1 = (a.n * a.m1 + b.n * b.m1) / combined.n;
		combined.m2 = a.m2 + b.m2 + delta2 * a.n * b.n / combined.n;

		return combined;
	}

	RunningStats& operator+=(const RunningStats& rhs)
	{
		RunningStats combined;
		combined = *this + rhs;

		*this = combined;
		return *this;
	}
};

template <typename Shape, typename Sampler>
void nullHypothesisTTestImpl(Shape shape, int numTests, int numSamples,
                             float significanceLevel, Sampler& sampler)
{
	RunningStats<float> stats;
	for(int index = 0; index < numSamples; ++index)
	{
		std::uint32_t out[2];
		sampler.sample(index, out);

		const auto x = oqmc::uintToFloat(out[0]);
		const auto y = oqmc::uintToFloat(out[1]);

		stats.push(shape.evaluate(x, y));
	}

	const auto result = hypothesis::students_t_test(
	    stats.mean(), stats.variance(), shape.integral(), numSamples,
	    significanceLevel, numTests);

	EXPECT_TRUE(result.first);
}

template <int Resolution, typename Sampler>
void nullHypothesisChiSquareImpl(int numTests, int numSamples,
                                 float significanceLevel, Sampler& sampler)
{
	constexpr auto numStrata = Resolution * Resolution;
	const auto totalSamples = numSamples * numStrata;

	std::array<double, numStrata> observations{};
	observations.fill(0.0);

	std::array<double, numStrata> expectations{};
	expectations.fill(numSamples);

	for(int index = 0; index < totalSamples; ++index)
	{
		std::uint32_t out[2];
		sampler.sample(index, out);

		const auto x = oqmc::uintToFloat(out[0]) * Resolution;
		const auto y = oqmc::uintToFloat(out[1]) * Resolution;

		const auto xInt =
		    std::min(std::max(static_cast<int>(x), 0), Resolution - 1);
		const auto yInt =
		    std::min(std::max(static_cast<int>(y), 0), Resolution - 1);

		const auto coordinate = xInt + yInt * Resolution;
		auto& strata = observations[coordinate];

		strata += 1.0;
	}

	const auto result = hypothesis::chi2_test(numStrata, observations.data(),
	                                          expectations.data(), totalSamples,
	                                          5, significanceLevel, numTests);

	EXPECT_TRUE(result.first);
}

template <int NumHeavisides, typename Sampler>
void nullHypothesisTTest(int numSeeds, int numSamples, float significanceLevel,
                         Sampler& sampler)
{
	const auto numTests = numSeeds * (7 + NumHeavisides);
	const auto heavisides = new OrientedHeaviside[NumHeavisides];

	OrientedHeaviside::build(NumHeavisides, heavisides);

	for(int seed = 0; seed < numSeeds; ++seed)
	{
		sampler.initialise(seed);
		nullHypothesisTTestImpl(QuarterDisk(), numTests, numSamples,
		                        significanceLevel, sampler);

		sampler.initialise(seed);
		nullHypothesisTTestImpl(FullDisk(), numTests, numSamples,
		                        significanceLevel, sampler);

		sampler.initialise(seed);
		nullHypothesisTTestImpl(QuarterGaussian(), numTests, numSamples,
		                        significanceLevel, sampler);

		sampler.initialise(seed);
		nullHypothesisTTestImpl(FullGaussian(), numTests, numSamples,
		                        significanceLevel, sampler);

		sampler.initialise(seed);
		nullHypothesisTTestImpl(Bilinear(), numTests, numSamples,
		                        significanceLevel, sampler);

		sampler.initialise(seed);
		nullHypothesisTTestImpl(LinearX(), numTests, numSamples,
		                        significanceLevel, sampler);

		sampler.initialise(seed);
		nullHypothesisTTestImpl(LinearY(), numTests, numSamples,
		                        significanceLevel, sampler);

		for(int i = 0; i < NumHeavisides; ++i)
		{
			sampler.initialise(seed);
			nullHypothesisTTestImpl(heavisides[i], numTests, numSamples,
			                        significanceLevel, sampler);
		}
	}

	delete[] heavisides;
}

template <int Resolution, typename Sampler>
void nullHypothesisChiSquare(int numSeeds, int numSamples,
                             float significanceLevel, Sampler& sampler)
{
	for(int seed = 0; seed < numSeeds; ++seed)
	{
		sampler.initialise(seed);
		nullHypothesisChiSquareImpl<Resolution>(numSeeds, numSamples,
		                                        significanceLevel, sampler);
	}
}

#define NULL_HYPOTHESIS_TTEST(name, description, functor)                      \
	TEST(name, tTest##description##Pattern)                                    \
	{                                                                          \
		auto sampler = functor;                                                \
		nullHypothesisTTest<defaultNumHeavisides>(                             \
		    defaultNumSeeds, defaultNumSamplesHigh, defaultSignificanceLevel,  \
		    sampler);                                                          \
	}

#define NULL_HYPOTHESIS_CHISQUARE(name, description, functor)                  \
	TEST(name, chiSquareTest##description##Pattern)                            \
	{                                                                          \
		auto sampler = functor;                                                \
		nullHypothesisChiSquare<defaultResolution>(                            \
		    defaultNumSeeds, defaultNumSamplesLow, defaultSignificanceLevel,   \
		    sampler);                                                          \
	}

#define ALL_HYPOTHESIS_TESTS(name, description, functor)                       \
	NULL_HYPOTHESIS_TTEST(name, description, functor)                          \
	NULL_HYPOTHESIS_CHISQUARE(name, description, functor)
