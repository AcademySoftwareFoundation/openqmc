// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "frequency.h"

#include "abi.h"

#if !defined(__CUDACC__)
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>
#endif

#include <algorithm>
#include <cassert>
#include <cmath>

namespace
{

constexpr double pi = 3.14159265358979323846;

float mean(const float x[], int size)
{
	assert(size >= 0);

	float mean = 0;
	for(int i = 0; i < size; ++i)
	{
		const auto delta = x[i] - mean;
		const auto deltaOverN = delta / (i + 1);

		mean += deltaOverN;
	}

	return mean;
}

void normalise(float x[], int size)
{
	assert(size > 0);

	float min = x[0];
	float max = x[0];
	for(int i = 1; i < size; ++i)
	{
		min = std::min(x[i], min);
		max = std::max(x[i], max);
	}

	for(int i = 0; i < size; ++i)
	{
		x[i] = (x[i] - min) / (max - min);
	}
}

} // namespace

// Based on 'Accurate Spectral Analysis of Two-Dimensional Point Sets' by
// Thomas Schlömer and Oliver Deussen. The oqmc_frequency_continuous algorithm
// computes the exact Fourier transform for a point set, removing the need to
// discretise the data into pixels.
OQMC_CABI bool oqmc_frequency_continuous(int nsequences, int nsamples,
                                         int ndims, int depthA, int depthB,
                                         int resolution, const float* in,
                                         float* out)
{
	assert(nsequences >= 0);
	assert(nsamples >= 0);
	assert(ndims >= 0);
	assert(depthA >= 0);
	assert(depthB >= 0);
	assert(resolution >= 0);
	assert(in);
	assert(out);

#if defined(__CUDACC__)
	auto func = [&](int begin, int end) {
		for(auto x = begin; x != end; ++x)
		{
#else
	auto func = [&](const oneapi::tbb::blocked_range<int>& r) {
		for(auto x = r.begin(); x != r.end(); ++x)
		{
#endif
			for(int y = 0; y < resolution; ++y)
			{
				const auto dx = x - resolution / 2.0f;
				const auto dy = y - resolution / 2.0f;

				float spectrum = 0.0f;
				for(int s = 0; s < nsequences; ++s)
				{
					float fx = 0.0f;
					float fy = 0.0f;
					for(int i = 0; i < nsamples; ++i)
					{
						const auto index = nsamples * ndims * s + ndims * i;

						const float x = in[index + depthA];
						const float y = in[index + depthB];

						const float exp = -pi * 2 * (dx * x + dy * y);

						fx += std::cos(exp);
						fy += std::sin(exp);
					}

					spectrum += (fx * fx + fy * fy) / nsamples;
				}

				const auto average = spectrum / nsequences;
				const auto tonemap = std::log2(1.0f + 0.5f * average);

				out[x + y * resolution] = tonemap;
			}
		}
	};

#if defined(__CUDACC__)
	func(0, resolution);
#else
	oneapi::tbb::parallel_for(oneapi::tbb::blocked_range<int>(0, resolution),
	                          func);
#endif

	return true;
}

OQMC_CABI bool oqmc_frequency_discrete_1d(int resolution, const float* inReal,
                                          const float* inImaginary,
                                          float* outReal, float* outImaginary)
{
	assert(resolution >= 0);
	assert(inReal);
	assert(inImaginary);
	assert(outReal);
	assert(outImaginary);

	const auto invResolution = 1.0f / resolution;

	for(int i = 0; i < resolution; ++i)
	{
		const auto constant = 2 * pi * i * invResolution;

		float sumReal = 0.0f;
		float sumImaginary = 0.0f;
		for(int j = 0; j < resolution; ++j)
		{
			const float cosine = std::cos(j * constant);
			const float sine = std::sin(j * constant);

			// NOLINTNEXTLINE: false positive
			sumReal += +inReal[j] * cosine + inImaginary[j] * sine;
			sumImaginary += -inReal[j] * sine + inImaginary[j] * cosine;
		}

		outReal[i] = sumReal * invResolution;
		outImaginary[i] = sumImaginary * invResolution;
	}

	return true;
}

OQMC_CABI bool oqmc_frequency_discrete_2d(int resolution, const float* in,
                                          float* out)
{
	assert(resolution >= 0);
	assert(in);
	assert(out);

	const auto npixels = resolution * resolution;
	const auto average = mean(in, npixels);

	auto realTemp1 = new float[npixels];
	auto realTemp2 = new float[npixels];
	auto imaginaryTemp1 = new float[npixels];
	auto imaginaryTemp2 = new float[npixels];

	for(int i = 0; i < npixels; ++i)
	{
		const auto x = i % resolution;
		const auto y = i / resolution;

		realTemp1[i] = (in[i] - average) * std::pow(-1.f, x + y);
		imaginaryTemp1[i] = 0.f;
	}

	for(int i = 0; i < resolution; ++i)
	{
		const auto index = i * resolution;

		oqmc_frequency_discrete_1d(resolution, &realTemp1[index],
		                           &imaginaryTemp1[index], &realTemp2[index],
		                           &imaginaryTemp2[index]);
	}

	for(int i = 0; i < npixels; ++i)
	{
		// NOLINTNEXTLINE: false positive
		const auto index = (i % resolution) * resolution + (i / resolution);

		realTemp1[i] = realTemp2[index];
		imaginaryTemp1[i] = imaginaryTemp2[index];
	}

	for(int i = 0; i < resolution; ++i)
	{
		const auto index = i * resolution;

		oqmc_frequency_discrete_1d(resolution, &realTemp1[index],
		                           &imaginaryTemp1[index], &realTemp2[index],
		                           &imaginaryTemp2[index]);
	}

	for(int i = 0; i < npixels; ++i)
	{
		const auto realSquared = realTemp2[i] * realTemp2[i];
		const auto imaginarySquared = imaginaryTemp2[i] * imaginaryTemp2[i];

		out[i] = std::log(std::sqrt(realSquared + imaginarySquared) + 1.f);
	}

	normalise(out, npixels);

	delete[] realTemp1;
	delete[] realTemp2;
	delete[] imaginaryTemp1;
	delete[] imaginaryTemp2;

	return true;
}

OQMC_CABI bool oqmc_frequency_discrete_3d(int resolution, int depth,
                                          const float* in, float* out)
{
	assert(resolution >= 0);
	assert(depth >= 0);
	assert(in);
	assert(out);

	const auto size = resolution * resolution;

	for(int i = 0; i < depth; ++i)
	{
		oqmc_frequency_discrete_2d(resolution, in + i * size, out + i * size);
	}

	return true;
}
