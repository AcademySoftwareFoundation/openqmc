// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "print.h"

#include <generate.h>

#include <cstdio>
#include <cstdlib>

float* start(int nsequences, int nsamples, int ndims)
{
	return new float[nsequences * nsamples * ndims];
}

void stop(const float* out)
{
	delete[] out;
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

	constexpr auto nsequences = 2;
	constexpr auto nsamples = 256;
	constexpr auto ndims = 8;

	auto out = start(nsequences, nsamples, ndims);

	if(!oqmc_generate(argv[1], nsequences, nsamples, ndims, out))
	{
		std::fprintf(stderr, "Sampler that was requested was not found; "
		                     "options are pmj, sobol, lattice.\n");

		goto failure;
	}

	print::csv(nsequences, nsamples, ndims, out);

	stop(out);
	return EXIT_SUCCESS;

failure:

	stop(out);
	return EXIT_FAILURE;
}
