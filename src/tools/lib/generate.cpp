// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "generate.h"

#include "parallel.h"
#include <oqmc/gpu.h>
#include <oqmc/oqmc.h>

#include <cassert>
#include <string>

namespace
{

struct Buffer
{
	float* points;
	void* cache;
};

OQMC_HOST_DEVICE int soaIndex(int indexId, int depthId, int nsamples)
{
	return indexId + depthId * nsamples;
}

OQMC_HOST_DEVICE int aosIndex(int indexId, int depthId, int ndims)
{
	return depthId + indexId * ndims;
}

template <typename Sampler>
OQMC_HOST_DEVICE void loop(int nsamples, int ndims, int seed, int index,
                           int stride, Buffer buffer)
{
	for(int i = index; i < nsamples; i += stride)
	{
		auto domain = Sampler(0, 0, 0, i, buffer.cache);

		for(int j = 0; j < ndims; j += 4)
		{
			domain = domain.newDomain(seed);

			float samples[4];
			domain.template drawSample<4>(samples);

			for(int k = 0; k < 4 && j + k < ndims; ++k)
			{
				buffer.points[soaIndex(i, j + k, nsamples)] = samples[k];
			}
		}
	}
}

#if defined(__CUDACC__)
template <typename Sampler>
__global__ void kernal(int nsamples, int ndims, int seed, Buffer buffer)
{
	const int index = blockIdx.x * blockDim.x + threadIdx.x;
	const int stride = blockDim.x * gridDim.x;

	loop<Sampler>(nsamples, ndims, seed, index, stride, buffer);
}
#else
template <typename Sampler>
void kernal(int nsamples, int ndims, int seed, Buffer buffer)
{
	const int index = 0;
	const int stride = 1;

	loop<Sampler>(nsamples, ndims, seed, index, stride, buffer);
}
#endif

void transpose(int nsamples, int ndims, const float* in, float* out)
{
	for(int i = 0; i < nsamples; ++i)
	{
		for(int j = 0; j < ndims; ++j)
		{
			const auto src = soaIndex(i, j, nsamples);
			const auto dst = aosIndex(i, j, ndims);

			out[dst] = in[src];
		}
	}
}

template <typename Sampler>
Buffer start(int nsequences, int nsamples, int ndims)
{
	Buffer ret;
	OQMC_ALLOCATE(&ret.points, nsequences * nsamples * ndims);
	OQMC_ALLOCATE(&ret.cache, Sampler::cacheSize);

	return ret;
}

void stop(Buffer out)
{
	OQMC_FREE(out.points);
	OQMC_FREE(out.cache);
}

template <typename Sampler>
void run(int nsequences, int nsamples, int ndims, float* out)
{
	auto buffer = start<Sampler>(nsequences, nsamples, ndims);

	Sampler::initialiseCache(buffer.cache);

	for(int i = 0; i < nsequences; ++i)
	{
		const auto offset = nsamples * ndims * i;

		auto ibuffer = Buffer{buffer.points + offset, buffer.cache};
		auto iout = out + offset;

		OQMC_LAUNCH(kernal<Sampler>, nsamples, ndims, i, ibuffer);
		transpose(nsamples, ndims, ibuffer.points, iout);
	}

	stop(buffer);
}

} // namespace

OQMC_CABI bool oqmc_generate(const char* name, int nsequences, int nsamples,
                             int ndims, float* out)
{
	assert(name);
	assert(nsequences >= 0);
	assert(nsamples >= 0);
	assert(ndims >= 0);
	assert(out);

	if(std::string(name) == "pmj")
	{
		run<oqmc::PmjSampler>(nsequences, nsamples, ndims, out);
		return true;
	}

	if(std::string(name) == "sobol")
	{
		run<oqmc::SobolSampler>(nsequences, nsamples, ndims, out);
		return true;
	}

	if(std::string(name) == "lattice")
	{
		run<oqmc::LatticeSampler>(nsequences, nsamples, ndims, out);
		return true;
	}

	return false;
}
