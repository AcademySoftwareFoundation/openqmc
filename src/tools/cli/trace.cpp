// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "write.h"

#include <trace.h>

#include <cstdio>
#include <cstdlib>

float3* start(int width, int height)
{
	return new float3[width * height];
}

void stop(float3* out)
{
	delete[] out;
}

int main(int argc, char* argv[])
{
	if(argc == 1)
	{
		std::fprintf(stderr, "No arguments passed; "
		                     "user must specify a sampler and a scene.\n");

		return EXIT_FAILURE;
	}

	if(argc < 3)
	{
		std::fprintf(stderr, "Too few arguments passed; "
		                     "user must specify a sampler and a scene.\n");

		return EXIT_FAILURE;
	}

	if(argc > 3)
	{
		std::fprintf(stderr, "Too many arguments passed; "
		                     "user must specify a sampler and a scene.\n");

		return EXIT_FAILURE;
	}

	constexpr auto mode = "split";
	constexpr auto width = 1080;
	constexpr auto height = 720;
	constexpr auto frame = 0;
	constexpr auto numPixelSamples = 1;
	constexpr auto numLightSamples = 1;
	constexpr auto maxDepth = 0;
	constexpr auto maxOpacity = 2;

	auto out = start(width, height);

	if(!oqmc_trace(argv[1], argv[2], mode, width, height, frame,
	               numPixelSamples, numLightSamples, maxDepth, maxOpacity, out))
	{
		std::fprintf(stderr, "Configuration that was requested was not found; "
		                     "sampler options are pmj, pmjbn, sobol, sobolbn, "
		                     "lattice, latticebn, rng; "
		                     "scene options are box, presence, blur.\n");

		goto failure;
	}

	write::colours("image.pfm", width, height, out);

	stop(out);
	return EXIT_SUCCESS;

failure:

	stop(out);
	return EXIT_FAILURE;
}
