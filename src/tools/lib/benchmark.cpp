// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "benchmark.h"

#include "parallel.h"
#include <oqmc/gpu.h>
#include <oqmc/oqmc.h>
#include <oqmc/unused.h>

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

namespace
{

template <typename Sampler>
OQMC_HOST_DEVICE void loop(int nsamples, int ndims, int index, int stride,
                           const void* cache)
{
	for(int i = index; i < nsamples; i += stride)
	{
		auto domain = Sampler(0, 0, 0, i, cache);

		for(int j = 0; j < ndims; j += 4)
		{
			domain = domain.newDomain(0);

			float sample[4];
			domain.template drawSample<4>(sample);

			volatile float save[4];
			save[0] = sample[0];
			save[1] = sample[1];
			save[2] = sample[2];
			save[3] = sample[3];

			OQMC_MAYBE_UNUSED(save);
		}
	}
}

#if defined(__CUDACC__)
template <typename Sampler>
__global__ void kernal(int nsamples, int ndims, const void* cache)
{
	const int index = blockIdx.x * blockDim.x + threadIdx.x;
	const int stride = blockDim.x * gridDim.x;

	loop<Sampler>(nsamples, ndims, index, stride, cache);
}
#else
template <typename Sampler>
void kernal(int nsamples, int ndims, const void* cache)
{
	const int index = 0;
	const int stride = 1;

	loop<Sampler>(nsamples, ndims, index, stride, cache);
}
#endif

template <typename Func>
int benchmark(Func run)
{
	using namespace std::chrono;

	const auto start = high_resolution_clock::now();

	run();

	const auto stop = high_resolution_clock::now();

	const auto duration = stop - start;
	const auto time = duration_cast<microseconds>(duration);

	return time.count();
}

template <typename Sampler>
bool run(const char* measurement, int nsamples, int ndims, int* out)
{
	void* cache;
	OQMC_ALLOCATE(&cache, Sampler::cacheSize);

	const auto timeInit =
	    benchmark([cache]() { Sampler::initialiseCache(cache); });

	const auto timeSamples = benchmark([nsamples, ndims, cache]() {
		OQMC_LAUNCH(kernal<Sampler>, nsamples, ndims, cache);
	});

	OQMC_FREE(cache);

	auto mesured = false;
	*out = 0;

	if(std::string(measurement) == "init")
	{
		mesured = true;
		*out += timeInit;
	}

	if(std::string(measurement) == "samples")
	{
		mesured = true;
		*out += timeSamples;
	}

	return mesured;
}

} // namespace

OQMC_CABI bool oqmc_benchmark(const char* sampler, const char* measurement,
                              int nsamples, int ndims, int* out)
{
	assert(sampler);
	assert(measurement);
	assert(nsamples >= 0);
	assert(ndims >= 0);
	assert(out);

	if(std::string(sampler) == "pmj")
	{
		return run<oqmc::PmjSampler>(measurement, nsamples, ndims, out);
	}

	if(std::string(sampler) == "pmjbn")
	{
		return run<oqmc::PmjBnSampler>(measurement, nsamples, ndims, out);
	}

	if(std::string(sampler) == "sobol")
	{
		return run<oqmc::SobolSampler>(measurement, nsamples, ndims, out);
	}

	if(std::string(sampler) == "sobolbn")
	{
		return run<oqmc::SobolBnSampler>(measurement, nsamples, ndims, out);
	}

	if(std::string(sampler) == "lattice")
	{
		return run<oqmc::LatticeSampler>(measurement, nsamples, ndims, out);
	}

	if(std::string(sampler) == "latticebn")
	{
		return run<oqmc::LatticeBnSampler>(measurement, nsamples, ndims, out);
	}

	return false;
}
