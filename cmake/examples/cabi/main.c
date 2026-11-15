// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

// Calls every function in the C ABI. Built as C11 in a C only project, this
// checks that the header is valid C, and that the library links from C with
// nothing beyond the C library. Values are compared against the C++ API in
// src/tests/oqmc_c.cpp.

#include <oqmc/oqmc_c.h>

#include <assert.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static_assert(sizeof(oqmc_sampler) == 16, "Value must be 16 bytes in size.");
static_assert(alignof(oqmc_sampler) == 8, "Value must have 8 byte alignment.");

static int failures = 0;

static void check(int condition, const char* message)
{
	if(!condition)
	{
		printf("FAILED: %s\n", message);
		++failures;
	}
}

static void checkDraws(const oqmc_sampler* sampler)
{
	uint32_t integers[4];
	float floats[4];

	oqmc_sampler_draw_sample_u(sampler, 4, integers);
	oqmc_sampler_draw_sample_f(sampler, 4, floats);

	for(int i = 0; i < 4; ++i)
	{
		check(floats[i] >= 0.0f && floats[i] < 1.0f, "sample in [0, 1)");
	}

	oqmc_sampler_draw_rnd_u(sampler, 4, integers);
	oqmc_sampler_draw_rnd_f(sampler, 4, floats);

	for(int i = 0; i < 4; ++i)
	{
		check(floats[i] >= 0.0f && floats[i] < 1.0f, "rnd in [0, 1)");
	}
}

int main(void)
{
	const char* name = oqmc_sampler_name();
	check(name != NULL && strlen(name) > 0, "name is not empty");

	const size_t cacheSize = oqmc_sampler_cache_size();
	void* cache = malloc(cacheSize);
	check(cacheSize == 0 || cache != NULL, "cache allocation");

	oqmc_sampler_initialise_cache(cache);

	oqmc_sampler sampler = oqmc_sampler_create(2, 3, 0, 0, cache);
	checkDraws(&sampler);

	sampler = oqmc_sampler_new_domain(&sampler, 1);
	checkDraws(&sampler);

	const oqmc_sampler split = oqmc_sampler_new_domain_split(&sampler, 2, 4, 3);
	checkDraws(&split);

	const oqmc_sampler distrib =
	    oqmc_sampler_new_domain_distrib(&sampler, 3, 1);
	checkDraws(&distrib);

	const oqmc_sampler chain = oqmc_sampler_new_domain_chain(&sampler, 4, 1);
	checkDraws(&chain);

	free(cache);

	if(failures > 0)
	{
		printf("%d check(s) failed for sampler '%s'\n", failures, name);
		return EXIT_FAILURE;
	}

	printf("All checks passed for sampler '%s'\n", name);
	return EXIT_SUCCESS;
}
