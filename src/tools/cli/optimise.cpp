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

Output start(int resolution, int depth)
{
	const auto npixels = resolution * resolution * depth;

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

	constexpr auto xBits = 6;
	constexpr auto yBits = 6;
	constexpr auto zBits = 4;

	static_assert(xBits == yBits,
	              "Optimisation tables have equal resolution in x and y");

	constexpr auto ntests = 8192;
	constexpr auto niterations = 262144;
	constexpr auto nsamples = 128;
	constexpr auto resolution = 1 << xBits;
	constexpr auto depth = 1 << zBits;
	constexpr auto seed = 0;

	auto out = start(resolution, depth);

	if(!oqmc_optimise(argv[1], ntests, niterations, nsamples, resolution, depth,
	                  seed, out.keys, out.ranks, out.estimates,
	                  out.frequencies))
	{
		std::fprintf(stderr, "Sampler that was requested was not found; "
		                     "options are pmj, sobol, lattice.\n");

		goto failure;
	}

	write::integers("keys.txt", resolution * resolution * depth, out.keys);
	write::integers("ranks.txt", resolution * resolution * depth, out.ranks);

	for(int i = 0; i < depth; ++i)
	{
		const auto estimatesName = "estimates" + std::to_string(i) + ".pfm";
		const auto frequenciesName = "frequencies" + std::to_string(i) + ".pfm";

		const auto offset = i * resolution * resolution;

		write::greyscales(estimatesName.c_str(), resolution, resolution,
		                  out.estimates + offset);

		write::greyscales(frequenciesName.c_str(), resolution, resolution,
		                  out.frequencies + offset);
	}

	stop(out);
	return EXIT_SUCCESS;

failure:

	stop(out);
	return EXIT_FAILURE;
}
