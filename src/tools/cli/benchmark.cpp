// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include <benchmark.h>

#include <cstdio>
#include <cstdlib>

int main(int argc, char* argv[])
{
	if(argc == 1)
	{
		std::fprintf(stderr,
		             "No arguments passed; "
		             "user must specify a sampler and a measurement.\n");

		return EXIT_FAILURE;
	}

	if(argc < 3)
	{
		std::fprintf(stderr,
		             "Too few arguments passed; "
		             "user must specify a sampler and a measurement.\n");

		return EXIT_FAILURE;
	}

	if(argc > 3)
	{
		std::fprintf(stderr,
		             "Too many arguments passed; "
		             "user must specify a sampler and a measurement.\n");

		return EXIT_FAILURE;
	}

	constexpr auto nsamples = 1 << 15; // 32k
	constexpr auto ndims = 256;

	int time;
	if(!oqmc_benchmark(argv[1], argv[2], nsamples, ndims, &time))
	{
		std::fprintf(stderr, "Configuration that was requested was not found; "
		                     "sampler options are pmj, pmjbn; "
		                     "measurement options are init, samples.\n");

		return EXIT_FAILURE;
	}

	std::printf("%i\n", time);

	return EXIT_SUCCESS;
}
