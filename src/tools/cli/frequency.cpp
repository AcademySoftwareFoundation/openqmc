// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "write.h"

#include <frequency.h>
#include <generate.h>

#include <cstdio>
#include <cstdlib>

struct Output
{
	float* samples;
	float* frequencies;
};

Output start(int nsequences, int nsamples, int ndims, int resolution)
{
	Output ret;
	ret.samples = new float[nsequences * nsamples * ndims];
	ret.frequencies = new float[resolution * resolution];

	return ret;
}

void stop(Output out)
{
	delete[] out.samples;
	delete[] out.frequencies;
}

int main(int argc, char* argv[])
{
	if(argc == 1)
	{
		std::fprintf(stderr, "No arguments passed; "
		                     "user must specify a sampler.\n");

		return EXIT_FAILURE;
	}

	if(argc > 2)
	{
		std::fprintf(stderr, "Too many arguments passed; "
		                     "user must specify a single sampler.\n");

		return EXIT_FAILURE;
	}

	constexpr auto nsequences = 128;
	constexpr auto nsamples = 256;
	constexpr auto ndims = 2;
	constexpr auto depthA = 0;
	constexpr auto depthB = 1;
	constexpr auto resolution = 128;

	auto out = start(nsequences, nsamples, ndims, resolution);

	if(!oqmc_generate(argv[1], nsequences, nsamples, ndims, out.samples))
	{
		std::fprintf(stderr, "Sampler that was requested was not found; "
		                     "options are pmj.\n");

		goto failure;
	}

	if(!oqmc_frequency_continuous(nsequences, nsamples, ndims, depthA, depthB,
	                              resolution, out.samples, out.frequencies))
	{
		std::fprintf(stderr, "DFT transform of generated samples failed; "
		                     "please investigate function for details.\n");

		goto failure;
	}

	write::greyscales("frequencies.pfm", resolution, resolution,
	                  out.frequencies);

	stop(out);
	return EXIT_SUCCESS;

failure:

	stop(out);
	return EXIT_FAILURE;
}
