// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "write.h"

#include <optimise.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

struct Output
{
	std::uint32_t* keys;
	std::uint32_t* ranks;
	float* estimates;
	float* frequencies;
};

Output start(int resolution)
{
	const auto npixels = resolution * resolution;

	Output ret;
	ret.keys = new std::uint32_t[npixels];
	ret.ranks = new std::uint32_t[npixels];
	ret.estimates = new float[npixels];
	ret.frequencies = new float[npixels];

	return ret;
}

void stop(Output out)
{
	delete[] out.keys;
	delete[] out.ranks;
	delete[] out.estimates;
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

	constexpr auto xBits = 8;
	constexpr auto yBits = 8;

	static_assert(xBits == yBits,
	              "Optimisation tables have equal resolution in x and y");

	constexpr auto ntests = 8192;
	constexpr auto niterations = 262144;
	constexpr auto nsamples = 32;
	constexpr auto resolution = 1 << xBits;
	constexpr auto seed = 0;

	auto out = start(resolution);

	if(!oqmc_optimise(argv[1], ntests, niterations, nsamples, resolution, seed,
	                  out.keys, out.ranks, out.estimates, out.frequencies))
	{
		std::fprintf(stderr, "Sampler that was requested was not found; "
		                     "options are pmj, sobol, lattice.\n");

		goto failure;
	}

	write::integers("keys.txt", resolution * resolution, out.keys);
	write::integers("ranks.txt", resolution * resolution, out.ranks);

	write::greyscales("estimates.pfm", resolution, resolution, out.estimates);
	write::greyscales("frequencies.pfm", resolution, resolution,
	                  out.frequencies);

	stop(out);
	return EXIT_SUCCESS;

failure:

	stop(out);
	return EXIT_FAILURE;
}
