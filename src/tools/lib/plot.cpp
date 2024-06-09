// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "plot.h"

#include "../../shapes.h"
#include "parallel.h"
#include <oqmc/float.h>
#include <oqmc/oqmc.h>
#include <oqmc/pcg.h>

#include <cmath>
#include <cstdint>
#include <string>

namespace
{

template <typename Shape>
void plotShape(Shape shape, int nsamples, int resolution, float* out)
{
	for(int x = 0; x < resolution; ++x)
	{
		for(int y = 0; y < resolution; ++y)
		{
			auto state = oqmc::pcg::init();

			float sum = 0;
			for(int i = 0; i < nsamples; ++i)
			{
				float rnd[2];
				rnd[0] = oqmc::uintToFloat(oqmc::pcg::rng(state));
				rnd[1] = oqmc::uintToFloat(oqmc::pcg::rng(state));

				const auto u = (x + rnd[0]) / resolution;
				const auto v = (y + rnd[1]) / resolution;

				sum += shape.evaluate(u, v);
			}

			out[x + resolution * y] = sum / nsamples;
		}
	}
}

template <typename Sampler>
void plotZoneplate(int nsamples, int resolution, float* out)
{
	void* cache;
	OQMC_ALLOCATE(&cache, Sampler::cacheSize);

	Sampler::initialiseCache(cache);

	for(int x = 0; x < resolution; ++x)
	{
		for(int y = 0; y < resolution; ++y)
		{
			float sum = 0;
			for(int i = 0; i < nsamples; ++i)
			{
				const auto domain = Sampler(x, y, 0, i, cache);

				float rnd[2];
				domain.template drawSample<2>(rnd);

				constexpr auto scale = 512;
				constexpr auto filterWidth = 2;

				const auto sampleTent = [](float radius, float u) {
					const auto sampleLinear = [](float u) {
						return 1 - std::sqrt(u);
					};

					if(u < 0.5f)
					{
						u = 1.0f - (u / 0.5f);
						return -radius * sampleLinear(u);
					}

					u = (u - 0.5f) / 0.5f;
					return +radius * sampleLinear(u);
				};

				float filterSample[2];
				filterSample[0] = sampleTent(filterWidth, rnd[0]);
				filterSample[1] = sampleTent(filterWidth, rnd[1]);

				const auto norm = 1.f / resolution;

				const auto u = (x + 0.5f) * norm + filterSample[0] * norm;
				const auto v = (y + 0.5f) * norm + filterSample[1] * norm;

				sum += 0.5f + 0.5f * std::cos(u * u * scale + v * v * scale);
			}

			out[x + resolution * y] = sum / nsamples;
		}
	}

	OQMC_FREE(cache);
}

template <typename Shape, typename Sampler>
void plotError(Shape shape, int nsequences, int nsamples, float* out)
{
	void* cache;
	OQMC_ALLOCATE(&cache, Sampler::cacheSize);

	float* buffer;
	OQMC_ALLOCATE(&buffer, nsequences);

	Sampler::initialiseCache(cache);

	for(int i = 0; i < nsequences; ++i)
	{
		buffer[i] = 0;
	}

	for(int index = 0; index < nsamples; ++index)
	{
		float sum = 0;
		for(int seed = 0; seed < nsequences; ++seed)
		{
			const auto domain = Sampler(0, 0, 0, index, cache).newDomain(seed);

			float rnd[2];
			domain.template drawSample<2>(rnd);

			const auto result = shape.evaluate(rnd[0], rnd[1]);

			buffer[seed] += result;

			const auto estimate = buffer[seed] / (index + 1);
			const auto error = estimate - shape.integral();

			sum += error * error;
		}

		out[index * 2 + 0] = index + 1;
		out[index * 2 + 1] = std::sqrt(sum / nsequences);
	}

	OQMC_FREE(cache);
	OQMC_FREE(buffer);
}

template <typename Shape>
bool plotError(const char* sampler, Shape shape, int nsequences, int nsamples,
               float* out)
{
	if(std::string(sampler) == "pmj")
	{
		plotError<Shape, oqmc::PmjSampler>(shape, nsequences, nsamples, out);

		return true;
	}

	if(std::string(sampler) == "sobol")
	{
		plotError<Shape, oqmc::SobolSampler>(shape, nsequences, nsamples, out);

		return true;
	}

	if(std::string(sampler) == "lattice")
	{
		plotError<Shape, oqmc::LatticeSampler>(shape, nsequences, nsamples,
		                                       out);

		return true;
	}

	return false;
}

template <typename Shape, typename Sampler>
void plotErrorFilterSpace(Shape shape, int resolution, int nsamples, int nsigma,
                          float sigmaMin, float sigmaStep, float* out)
{
	const auto npixels = resolution * resolution;

	void* cache;
	OQMC_ALLOCATE(&cache, Sampler::cacheSize);

	float* imageA;
	OQMC_ALLOCATE(&imageA, npixels);

	float* imageB;
	OQMC_ALLOCATE(&imageB, npixels);

	Sampler::initialiseCache(cache);

	for(int x = 0; x < resolution; ++x)
	{
		for(int y = 0; y < resolution; ++y)
		{
			float estimate = 0;
			for(int index = 0; index < nsamples; ++index)
			{
				const auto domain = Sampler(x, y, 0, index, cache);

				float rnd[2];
				domain.template drawSample<2>(rnd);

				estimate += shape.evaluate(rnd[0], rnd[1]);
			}

			imageA[y + x * resolution] = estimate / nsamples;
		}
	}

	for(int s = 0; s < nsigma; ++s)
	{
		const auto sigma = sigmaMin + s * sigmaStep;

		constexpr auto radius = 32;
		constexpr auto width = radius * 2 + 1;

		float kernel[width];

		for(int i = 0; i < width; ++i)
		{
			const auto x = i - radius;

			kernel[i] = std::exp(-(x * x) / (2 * sigma * sigma));
		}

		for(int px = 0; px < resolution; ++px)
		{
			for(int py = 0; py < resolution; ++py)
			{
				const int p = py + px * resolution;

				float sum = 0;
				float weightSum = 0;
				for(int x = 0; x < width; ++x)
				{
					for(int y = 0; y < width; ++y)
					{
						const int qx = px - radius + x;
						const int qy = py - radius + y;

						if(qx < 0 || qy < 0 || qx >= resolution ||
						   qy >= resolution)
						{
							continue;
						}

						const int q = qy + qx * resolution;

						sum += imageA[q] * kernel[x] * kernel[y];
						weightSum += kernel[x] * kernel[y];
					}
				}

				imageB[p] = sum / weightSum;
			}
		}

		float sum = 0;
		for(int x = 0; x < resolution; ++x)
		{
			for(int y = 0; y < resolution; ++y)
			{
				const auto error =
				    imageB[y + x * resolution] - shape.integral();

				sum += error * error;
			}
		}

		out[s * 2 + 0] = sigma;
		out[s * 2 + 1] = std::sqrt(sum / npixels);
	}

	OQMC_FREE(cache);
	OQMC_FREE(imageA);
	OQMC_FREE(imageB);
}

template <typename Shape>
bool plotErrorFilterSpace(const char* sampler, Shape shape, int resolution,
                          int nsamples, int nsigma, float sigmaMin,
                          float sigmaStep, float* out)
{
	if(std::string(sampler) == "pmj")
	{
		plotErrorFilterSpace<Shape, oqmc::PmjSampler>(
		    shape, resolution, nsamples, nsigma, sigmaMin, sigmaStep, out);

		return true;
	}

	if(std::string(sampler) == "pmjbn")
	{
		plotErrorFilterSpace<Shape, oqmc::PmjBnSampler>(
		    shape, resolution, nsamples, nsigma, sigmaMin, sigmaStep, out);

		return true;
	}

	if(std::string(sampler) == "sobol")
	{
		plotErrorFilterSpace<Shape, oqmc::SobolSampler>(
		    shape, resolution, nsamples, nsigma, sigmaMin, sigmaStep, out);

		return true;
	}

	if(std::string(sampler) == "sobolbn")
	{
		plotErrorFilterSpace<Shape, oqmc::SobolBnSampler>(
		    shape, resolution, nsamples, nsigma, sigmaMin, sigmaStep, out);

		return true;
	}

	if(std::string(sampler) == "lattice")
	{
		plotErrorFilterSpace<Shape, oqmc::LatticeSampler>(
		    shape, resolution, nsamples, nsigma, sigmaMin, sigmaStep, out);

		return true;
	}

	if(std::string(sampler) == "latticebn")
	{
		plotErrorFilterSpace<Shape, oqmc::LatticeBnSampler>(
		    shape, resolution, nsamples, nsigma, sigmaMin, sigmaStep, out);

		return true;
	}

	return false;
}

template <typename Shape, typename Sampler>
void plotErrorFilterTime(Shape shape, int resolution, int depth, int nsamples,
                         int nsigma, float sigmaMin, float sigmaStep,
                         float* out)
{
	const auto npixels = resolution * depth;

	void* cache;
	OQMC_ALLOCATE(&cache, Sampler::cacheSize);

	float* imageA;
	OQMC_ALLOCATE(&imageA, npixels);

	float* imageB;
	OQMC_ALLOCATE(&imageB, npixels);

	Sampler::initialiseCache(cache);

	for(int x = 0; x < resolution; ++x)
	{
		for(int z = 0; z < depth; ++z)
		{
			float estimate = 0;
			for(int index = 0; index < nsamples; ++index)
			{
				const auto domain = Sampler(x, 0, z, index, cache);

				float rnd[2];
				domain.template drawSample<2>(rnd);

				estimate += shape.evaluate(rnd[0], rnd[1]);
			}

			imageA[z + x * depth] = estimate / nsamples;
		}
	}

	for(int s = 0; s < nsigma; ++s)
	{
		const auto sigma = sigmaMin + s * sigmaStep;

		constexpr auto radius = 32;
		constexpr auto width = radius * 2 + 1;

		float kernel[width];

		for(int i = 0; i < width; ++i)
		{
			const auto x = i - radius;

			kernel[i] = std::exp(-(x * x) / (2 * sigma * sigma));
		}

		for(int px = 0; px < resolution; ++px)
		{
			for(int pz = 0; pz < depth; ++pz)
			{
				const int p = pz + px * depth;

				float sum = 0;
				float weightSum = 0;
				for(int z = 0; z < width; ++z)
				{
					const int qx = px;
					const int qz = pz - radius + z;

					if(qx < 0 || qz < 0 || qx >= resolution || qz >= depth)
					{
						continue;
					}

					const int q = qz + qx * depth;

					sum += imageA[q] * kernel[z];
					weightSum += kernel[z];
				}

				imageB[p] = sum / weightSum;
			}
		}

		float sum = 0;
		for(int x = 0; x < resolution; ++x)
		{
			for(int z = 0; z < depth; ++z)
			{
				const auto error = imageB[z + x * depth] - shape.integral();

				sum += error * error;
			}
		}

		out[s * 2 + 0] = sigma;
		out[s * 2 + 1] = std::sqrt(sum / npixels);
	}

	OQMC_FREE(cache);
	OQMC_FREE(imageA);
	OQMC_FREE(imageB);
}

template <typename Shape>
bool plotErrorFilterTime(const char* sampler, Shape shape, int resolution,
                         int depth, int nsamples, int nsigma, float sigmaMin,
                         float sigmaStep, float* out)
{
	if(std::string(sampler) == "pmj")
	{
		plotErrorFilterTime<Shape, oqmc::PmjSampler>(shape, resolution, depth,
		                                             nsamples, nsigma, sigmaMin,
		                                             sigmaStep, out);

		return true;
	}

	if(std::string(sampler) == "pmjbn")
	{
		plotErrorFilterTime<Shape, oqmc::PmjBnSampler>(
		    shape, resolution, depth, nsamples, nsigma, sigmaMin, sigmaStep,
		    out);

		return true;
	}

	if(std::string(sampler) == "sobol")
	{
		plotErrorFilterTime<Shape, oqmc::SobolSampler>(
		    shape, resolution, depth, nsamples, nsigma, sigmaMin, sigmaStep,
		    out);

		return true;
	}

	if(std::string(sampler) == "sobolbn")
	{
		plotErrorFilterTime<Shape, oqmc::SobolBnSampler>(
		    shape, resolution, depth, nsamples, nsigma, sigmaMin, sigmaStep,
		    out);

		return true;
	}

	if(std::string(sampler) == "lattice")
	{
		plotErrorFilterTime<Shape, oqmc::LatticeSampler>(
		    shape, resolution, depth, nsamples, nsigma, sigmaMin, sigmaStep,
		    out);

		return true;
	}

	if(std::string(sampler) == "latticebn")
	{
		plotErrorFilterTime<Shape, oqmc::LatticeBnSampler>(
		    shape, resolution, depth, nsamples, nsigma, sigmaMin, sigmaStep,
		    out);

		return true;
	}

	return false;
}

const auto orientedHeaviside = OrientedHeaviside(0.333f, 0.65f, 0.525f);

} // namespace

OQMC_CABI bool oqmc_plot_shape(const char* shape, int nsamples, int resolution,
                               float* out)
{
	assert(shape);
	assert(nsamples >= 0);
	assert(resolution >= 0);
	assert(out);

	if(std::string(shape) == "qdisk")
	{
		plotShape(QuarterDisk(), nsamples, resolution, out);

		return true;
	}

	if(std::string(shape) == "fdisk")
	{
		plotShape(FullDisk(), nsamples, resolution, out);

		return true;
	}

	if(std::string(shape) == "qgauss")
	{
		plotShape(QuarterGaussian(), nsamples, resolution, out);

		return true;
	}

	if(std::string(shape) == "fgauss")
	{
		plotShape(FullGaussian(), nsamples, resolution, out);

		return true;
	}

	if(std::string(shape) == "bilin")
	{
		plotShape(Bilinear(), nsamples, resolution, out);

		return true;
	}

	if(std::string(shape) == "linx")
	{
		plotShape(LinearX(), nsamples, resolution, out);

		return true;
	}

	if(std::string(shape) == "liny")
	{
		plotShape(LinearY(), nsamples, resolution, out);

		return true;
	}

	if(std::string(shape) == "heavi")
	{
		plotShape(orientedHeaviside, nsamples, resolution, out);

		return true;
	}

	return false;
}

OQMC_CABI bool oqmc_plot_zoneplate(const char* sampler, int nsamples,
                                   int resolution, float* out)
{
	assert(sampler);
	assert(nsamples >= 0);
	assert(resolution >= 0);
	assert(out);

	if(std::string(sampler) == "pmj")
	{
		plotZoneplate<oqmc::PmjSampler>(nsamples, resolution, out);

		return true;
	}

	if(std::string(sampler) == "pmjbn")
	{
		plotZoneplate<oqmc::PmjBnSampler>(nsamples, resolution, out);

		return true;
	}

	if(std::string(sampler) == "sobol")
	{
		plotZoneplate<oqmc::SobolSampler>(nsamples, resolution, out);

		return true;
	}

	if(std::string(sampler) == "sobolbn")
	{
		plotZoneplate<oqmc::SobolBnSampler>(nsamples, resolution, out);

		return true;
	}

	if(std::string(sampler) == "lattice")
	{
		plotZoneplate<oqmc::LatticeSampler>(nsamples, resolution, out);

		return true;
	}

	if(std::string(sampler) == "latticebn")
	{
		plotZoneplate<oqmc::LatticeBnSampler>(nsamples, resolution, out);

		return true;
	}

	return false;
}

OQMC_CABI bool oqmc_plot_error(const char* shape, const char* sampler,
                               int nsequences, int nsamples, float* out)
{
	assert(shape);
	assert(sampler);
	assert(nsequences >= 0);
	assert(nsamples >= 0);
	assert(out);

	if(std::string(shape) == "qdisk")
	{
		return plotError(sampler, QuarterDisk(), nsequences, nsamples, out);
	}

	if(std::string(shape) == "fdisk")
	{
		return plotError(sampler, FullDisk(), nsequences, nsamples, out);
	}

	if(std::string(shape) == "qgauss")
	{
		return plotError(sampler, QuarterGaussian(), nsequences, nsamples, out);
	}

	if(std::string(shape) == "fgauss")
	{
		return plotError(sampler, FullGaussian(), nsequences, nsamples, out);
	}

	if(std::string(shape) == "bilin")
	{
		return plotError(sampler, Bilinear(), nsequences, nsamples, out);
	}

	if(std::string(shape) == "linx")
	{
		return plotError(sampler, LinearX(), nsequences, nsamples, out);
	}

	if(std::string(shape) == "liny")
	{
		return plotError(sampler, LinearY(), nsequences, nsamples, out);
	}

	if(std::string(shape) == "heavi")
	{
		return plotError(sampler, orientedHeaviside, nsequences, nsamples, out);
	}

	return false;
}

OQMC_CABI bool oqmc_plot_error_filter_space(const char* shape,
                                            const char* sampler, int resolution,
                                            int nsamples, int nsigma,
                                            float sigmaMin, float sigmaStep,
                                            float* out)
{
	assert(shape);
	assert(sampler);
	assert(resolution >= 0);
	assert(nsamples >= 0);
	assert(nsigma >= 0);
	assert(sigmaMin > 0);
	assert(sigmaStep > 0);
	assert(out);

	if(std::string(shape) == "qdisk")
	{
		return plotErrorFilterSpace(sampler, QuarterDisk(), resolution,
		                            nsamples, nsigma, sigmaMin, sigmaStep, out);
	}

	if(std::string(shape) == "fdisk")
	{
		return plotErrorFilterSpace(sampler, FullDisk(), resolution, nsamples,
		                            nsigma, sigmaMin, sigmaStep, out);
	}

	if(std::string(shape) == "qgauss")
	{
		return plotErrorFilterSpace(sampler, QuarterGaussian(), resolution,
		                            nsamples, nsigma, sigmaMin, sigmaStep, out);
	}

	if(std::string(shape) == "fgauss")
	{
		return plotErrorFilterSpace(sampler, FullGaussian(), resolution,
		                            nsamples, nsigma, sigmaMin, sigmaStep, out);
	}

	if(std::string(shape) == "bilin")
	{
		return plotErrorFilterSpace(sampler, Bilinear(), resolution, nsamples,
		                            nsigma, sigmaMin, sigmaStep, out);
	}

	if(std::string(shape) == "linx")
	{
		return plotErrorFilterSpace(sampler, LinearX(), resolution, nsamples,
		                            nsigma, sigmaMin, sigmaStep, out);
	}

	if(std::string(shape) == "liny")
	{
		return plotErrorFilterSpace(sampler, LinearY(), resolution, nsamples,
		                            nsigma, sigmaMin, sigmaStep, out);
	}

	if(std::string(shape) == "heavi")
	{
		return plotErrorFilterSpace(sampler, orientedHeaviside, resolution,
		                            nsamples, nsigma, sigmaMin, sigmaStep, out);
	}

	return false;
}

OQMC_CABI bool oqmc_plot_error_filter_time(const char* shape,
                                           const char* sampler, int resolution,
                                           int depth, int nsamples, int nsigma,
                                           float sigmaMin, float sigmaStep,
                                           float* out)
{
	assert(shape);
	assert(sampler);
	assert(resolution >= 0);
	assert(nsamples >= 0);
	assert(nsigma >= 0);
	assert(sigmaMin > 0);
	assert(sigmaStep > 0);
	assert(out);

	if(std::string(shape) == "qdisk")
	{
		return plotErrorFilterTime(sampler, QuarterDisk(), resolution, depth,
		                           nsamples, nsigma, sigmaMin, sigmaStep, out);
	}

	if(std::string(shape) == "fdisk")
	{
		return plotErrorFilterTime(sampler, FullDisk(), resolution, depth,
		                           nsamples, nsigma, sigmaMin, sigmaStep, out);
	}

	if(std::string(shape) == "qgauss")
	{
		return plotErrorFilterTime(sampler, QuarterGaussian(), resolution,
		                           depth, nsamples, nsigma, sigmaMin, sigmaStep,
		                           out);
	}

	if(std::string(shape) == "fgauss")
	{
		return plotErrorFilterTime(sampler, FullGaussian(), resolution, depth,
		                           nsamples, nsigma, sigmaMin, sigmaStep, out);
	}

	if(std::string(shape) == "bilin")
	{
		return plotErrorFilterTime(sampler, Bilinear(), resolution, depth,
		                           nsamples, nsigma, sigmaMin, sigmaStep, out);
	}

	if(std::string(shape) == "linx")
	{
		return plotErrorFilterTime(sampler, LinearX(), resolution, depth,
		                           nsamples, nsigma, sigmaMin, sigmaStep, out);
	}

	if(std::string(shape) == "liny")
	{
		return plotErrorFilterTime(sampler, LinearY(), resolution, depth,
		                           nsamples, nsigma, sigmaMin, sigmaStep, out);
	}

	if(std::string(shape) == "heavi")
	{
		return plotErrorFilterTime(sampler, orientedHeaviside, resolution,
		                           depth, nsamples, nsigma, sigmaMin, sigmaStep,
		                           out);
	}

	return false;
}
