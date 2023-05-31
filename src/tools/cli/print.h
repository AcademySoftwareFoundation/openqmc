// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#pragma once

#include <cstdio>

namespace print
{

inline void csv(int nsequences, int nsamples, int ndims, const float* points)
{
	for(int i = 0; i < nsequences; ++i)
	{
		for(int j = 0; j < nsamples; ++j)
		{
			for(int k = 0; k < ndims; ++k)
			{
				const auto index = k + j * ndims;

				if(k > 0)
				{
					printf(",");
				}

				std::printf("%f", points[index]);
			}

			std::printf("\n");
		}

		points += nsamples * ndims;
	}
}

} // namespace print
