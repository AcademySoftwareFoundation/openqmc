// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#pragma once

#include <vector.h>

#include <cstdint>
#include <cstdio>

namespace write
{

inline void greyscales(const char* name, int xres, int yres, const float* image)
{
	auto file = std::fopen(name, "w");

	if(file == nullptr)
	{
		return;
	}

	std::fprintf(file, "Pf\n");
	std::fprintf(file, "%i %i\n", xres, yres);
	std::fprintf(file, "-1\n");
	std::fwrite(image, sizeof(float), xres * yres, file);

	std::fclose(file);
}

inline void colours(const char* name, int xres, int yres, const float3* image)
{
	auto file = std::fopen(name, "w");

	if(file == nullptr)
	{
		return;
	}

	std::fprintf(file, "PF\n");
	std::fprintf(file, "%i %i\n", xres, yres);
	std::fprintf(file, "-1\n");
	std::fwrite(image, sizeof(float3), xres * yres, file);

	std::fclose(file);
}

inline void integers(const char* name, int size, const std::uint32_t* integers)
{
	auto file = std::fopen(name, "w");

	if(file == nullptr)
	{
		return;
	}

	for(int i = 0; i < size; ++i)
	{
		std::fprintf(file, "0x%08xU,\n", integers[i]);
	}

	std::fclose(file);
}

} // namespace write
