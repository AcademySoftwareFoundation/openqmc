// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#pragma once

#if !defined(__CUDACC__)

// NOLINTNEXTLINE: C style naming
struct float2
{
	float x;
	float y;
};

// NOLINTNEXTLINE: C style naming
struct float3
{
	float x;
	float y;
	float z;
};

// NOLINTNEXTLINE: C style naming
struct float4
{
	float x;
	float y;
	float z;
	float w;
};

// NOLINTNEXTLINE: C style naming
struct int2
{
	int x;
	int y;
};

// NOLINTNEXTLINE: C style naming
struct int3
{
	int x;
	int y;
	int z;
};

// NOLINTNEXTLINE: C style naming
struct int4
{
	int x;
	int y;
	int z;
	int w;
};

#endif
