// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "progress.h"

#include "abi.h"

#include <cassert>
#include <cstdio>
#include <ctime>

namespace
{

void compute(const char* string, long int size, long int index,
             std::time_t start)
{
	const float progress = 1.0f / size * index;

	const int markDone = 32 * progress;
	const int markLeft = 32 - markDone;

	std::fprintf(stderr, "\r%-20.20s [", string);

	for(int i = 0; i < markDone; ++i)
	{
		std::fprintf(stderr, "+");
	}

	for(int i = 0; i < markLeft; ++i)
	{
		std::fprintf(stderr, " ");
	}

	std::fprintf(stderr, "] %06.2f%%", progress * 100);

	const auto current = std::time(nullptr);

	const int past = current - start;
	const int total = past / progress;
	const int future = total - past;

	if(index > 0)
	{
		std::fprintf(stderr, " (p: %05is, f: %05is, t: %05is)", past, future,
		             total);
	}

	std::fflush(stdout);
}

bool enabled = true;

} // namespace

OQMC_CABI void oqmc_progress_on()
{
	enabled = true;
}

OQMC_CABI void oqmc_progress_off()
{
	enabled = false;
}

OQMC_CABI time_t oqmc_progress_start(const char* string, long int size)
{
	assert(string);
	assert(size >= 0);

	if(enabled)
	{
		compute(string, size, 0, 0);
	}

	return std::time(nullptr);
}

OQMC_CABI void oqmc_progress_end()
{
	if(enabled)
	{
		std::fprintf(stderr, "\n");
	}
}

OQMC_CABI void oqmc_progress_add(const char* string, long int size,
                                 long int index, time_t start)
{
	assert(string);
	assert(size >= 0);
	assert(index >= 0);

	if(enabled)
	{
		compute(string, size, index, start);
	}
}
