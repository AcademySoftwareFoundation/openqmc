// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include <oqmc/oqmc.h>

int main()
{
	void* cache = new char[oqmc::PmjSampler::cacheSize];
	oqmc::PmjSampler::initialiseCache(cache);
	delete[] static_cast<char*>(cache);

	return 0;
}
