// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#pragma once

#include <oqmc/float.h>
#include <oqmc/gpu.h>
#include <oqmc/pcg.h>
#include <oqmc/unused.h>

#include <cmath>

// These 2D test functions (shapes) were inspired by the Per Christensen's
// funcsamp2D (https://github.com/perchristensen/funcsamp2D) repo. They are
// used to measure the error of estimates against a known value when testing,
// as well as during blue noise optimisation.

struct QuarterDisk
{
	OQMC_HOST_DEVICE static float evaluate(float x, float y)
	{
		return x * x + y * y < 2 / M_PI ? 1 : 0;
	}

	OQMC_HOST_DEVICE static float integral()
	{
		return 0.5f;
	}
};

struct FullDisk
{
	OQMC_HOST_DEVICE static float evaluate(float x, float y)
	{
		x = x - 0.5f;
		y = y - 0.5f;

		return x * x + y * y < 1 / (2 * M_PI) ? 1 : 0;
	}

	OQMC_HOST_DEVICE static float integral()
	{
		return 0.5f;
	}
};

struct QuarterGaussian
{
	OQMC_HOST_DEVICE static float evaluate(float x, float y)
	{
		return std::exp(-(x * x + y * y));
	}

	OQMC_HOST_DEVICE static float integral()
	{
		return M_PI_4 * std::pow(std::erf(1.0f), 2.0f);
	}
};

struct FullGaussian
{
	OQMC_HOST_DEVICE static float evaluate(float x, float y)
	{
		x = x - 0.5f;
		y = y - 0.5f;

		return std::exp(-(x * x + y * y));
	}

	OQMC_HOST_DEVICE static float integral()
	{
		return M_PI * std::pow(std::erff(0.5f), 2.0f);
	}
};

struct Bilinear
{
	OQMC_HOST_DEVICE static float evaluate(float x, float y)
	{
		return x * y;
	}

	OQMC_HOST_DEVICE static float integral()
	{
		return 0.25f;
	}
};

struct LinearX
{
	OQMC_HOST_DEVICE static float evaluate(float x, float y)
	{
		OQMC_MAYBE_UNUSED(y);

		return x;
	}

	OQMC_HOST_DEVICE static float integral()
	{
		return 0.5f;
	}
};

struct LinearY
{
	OQMC_HOST_DEVICE static float evaluate(float x, float y)
	{
		OQMC_MAYBE_UNUSED(x);

		return y;
	}

	OQMC_HOST_DEVICE static float integral()
	{
		return 0.5f;
	}
};

struct OrientedHeaviside
{
	// NOLINTNEXTLINE: C style naming
	struct float2
	{
		float x;
		float y;
	};

	/*AUTO_DEFINED*/ OrientedHeaviside() = default;
	OQMC_HOST_DEVICE OrientedHeaviside(float orientation, float x, float y)
	{
		const float theta = 2 * M_PI * orientation;

		pos.x = x;
		pos.y = y;
		normal.x = std::cos(theta);
		normal.y = std::sin(theta);
	}

	static void build(int size, OrientedHeaviside* heavisides)
	{
		auto state = oqmc::pcg::init(12345);

		for(int i = 0; i < size; ++i)
		{
			float rnd[3];

			rnd[0] = oqmc::uintToFloat(oqmc::pcg::rng(state));
			rnd[1] = oqmc::uintToFloat(oqmc::pcg::rng(state));
			rnd[2] = oqmc::uintToFloat(oqmc::pcg::rng(state));

			heavisides[i] = OrientedHeaviside(rnd[0], rnd[1], rnd[2]);
		}
	}

	OQMC_HOST_DEVICE float evaluate(float x, float y) const
	{
		x = x - pos.x;
		y = y - pos.y;

		return x * normal.x + y * normal.y < 0 ? 1 : 0;
	}

	OQMC_HOST_DEVICE float integral() const
	{
		struct SlopeIntercept
		{
			OQMC_HOST_DEVICE SlopeIntercept(OrientedHeaviside heaviside)
			{
				auto orthogonalVector = [](float2 vector) {
					return float2{-vector.y, +vector.x};
				};

				auto vectorSlope = [](float2 vector) {
					return vector.y / vector.x;
				};

				const auto negativeX = -heaviside.pos.x;
				const auto positiveY = +heaviside.pos.y;

				a = vectorSlope(orthogonalVector(heaviside.normal));
				b = a * negativeX + positiveY;
			}

			OQMC_HOST_DEVICE float linearFunctionFwd(float x) const
			{
				return a * x + b;
			}

			OQMC_HOST_DEVICE float linearFunctionInv(float y) const
			{
				return (y - b) / a;
			}

			float a;
			float b;
		};

		auto inZeroOneSegment = [](float t) { return t >= 0 && t < 1; };
		auto areaRightTriangle = [](float a, float b) { return a * b / 2; };
		auto areaRightTrapezoid = [](float a, float h1, float h2) {
			return a * (h1 + h2) / 2;
		};

		const auto line = SlopeIntercept(*this);
		const auto x0 = line.linearFunctionInv(0);
		const auto x1 = line.linearFunctionInv(1);
		const auto y0 = line.linearFunctionFwd(0);
		const auto y1 = line.linearFunctionFwd(1);

		if(inZeroOneSegment(x0) && inZeroOneSegment(x1))
		{
			auto area = areaRightTrapezoid(1, x0, x1);

			if(normal.x < 0)
			{
				area = 1 - area;
			}

			return area;
		}

		if(inZeroOneSegment(y0) && inZeroOneSegment(y1))
		{
			auto area = areaRightTrapezoid(1, y0, y1);

			if(normal.y < 0)
			{
				area = 1 - area;
			}

			return area;
		}

		if(inZeroOneSegment(x0) && inZeroOneSegment(y0))
		{
			auto area = areaRightTriangle(x0, y0);

			if(normal.x < 0 || normal.y < 0)
			{
				area = 1 - area;
			}

			return area;
		}

		if(inZeroOneSegment(x1) && inZeroOneSegment(y1))
		{
			auto area = areaRightTriangle(1 - x1, 1 - y1);

			if(normal.x > 0 || normal.y > 0)
			{
				area = 1 - area;
			}

			return area;
		}

		if(inZeroOneSegment(x0) && inZeroOneSegment(y1))
		{
			auto area = areaRightTriangle(1 - x0, y1);

			if(normal.x > 0 || normal.y < 0)
			{
				area = 1 - area;
			}

			return area;
		}

		if(inZeroOneSegment(x1) && inZeroOneSegment(y0))
		{
			auto area = areaRightTriangle(x1, 1 - y0);

			if(normal.x < 0 || normal.y > 0)
			{
				area = 1 - area;
			}

			return area;
		}

		return 0;
	}

	float2 pos;
	float2 normal;
};
