// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "write.h"

#include <plot.h>

#include <cstdio>
#include <cstdlib>

float* start(int resolution)
{
	return new float[resolution * resolution];
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
		                     "user must specify a shape.\n");

		return EXIT_FAILURE;
	}

	if(argc > 2)
	{
		std::fprintf(stderr, "Too many arguments passed; "
		                     "user must specify a shape.\n");

		return EXIT_FAILURE;
	}

	constexpr auto nsamples = 8;
	constexpr auto resolution = 256;

	auto out = start(resolution);

	if(!oqmc_plot_shape(argv[1], nsamples, resolution, out))
	{
		std::fprintf(stderr, "Shape that was requested was not found; "
		                     "options are qdisk, fdisk, qgauss, fgauss, bilin, "
		                     "linx, liny, heavi.\n");

		goto failure;
	}

	write::greyscales("shape.pfm", resolution, resolution, out);

	stop(out);
	return EXIT_SUCCESS;

failure:

	stop(out);
	return EXIT_FAILURE;
}
