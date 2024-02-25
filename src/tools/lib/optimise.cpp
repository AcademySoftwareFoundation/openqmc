// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "optimise.h"

#include "../../shapes.h"
#include "frequency.h"
#include "parallel.h"
#include "progress.h"
#include "vector.h"

#include <oqmc/float.h>
#include <oqmc/gpu.h>
#include <oqmc/lookup.h>
#include <oqmc/owen.h>
#include <oqmc/pcg.h>
#include <oqmc/rank1.h>
#include <oqmc/state.h>
#include <oqmc/stochastic.h>
#include <oqmc/unused.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace
{

constexpr auto transactionSize = 262144; // found to be good for an A6000 GPU.

OQMC_HOST_DEVICE bool isPowerOfTwo(int x)
{
	assert(x >= 0);

	return (x != 0) && ((x & (x - 1)) == 0);
}

OQMC_HOST_DEVICE int bitMask(int powerOfTwo)
{
	assert(isPowerOfTwo(powerOfTwo));

	return powerOfTwo - 1;
}

OQMC_HOST_DEVICE bool operator==(const int3& a, const int3& b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

template <typename T>
OQMC_HOST_DEVICE void swap(T& a, T& b)
{
	const auto temp = a;
	a = b;
	b = temp;
}

// Provides indexing services into a 3D grid given a grid size. Using this
// type the caller can allocate an array of appropriate capacity to hold all
// elements, and index in and out of the array using XYZ coordinates.
struct Array3d
{
	OQMC_HOST_DEVICE Array3d(int3 shape);
	OQMC_HOST_DEVICE int size() const;
	OQMC_HOST_DEVICE int index(int3 coordinate) const;
	OQMC_HOST_DEVICE int3 coordinate(int index) const;

	const int3 shape;
};

Array3d::Array3d(int3 shape) : shape(shape)
{
	assert(shape.x > 0);
	assert(shape.y > 0);
	assert(shape.z > 0);
}

int Array3d::size() const
{
	return shape.x * shape.y * shape.z;
}

int Array3d::index(int3 coordinate) const
{
	assert(coordinate.x >= 0);
	assert(coordinate.y >= 0);
	assert(coordinate.z >= 0);
	assert(coordinate.x < shape.x);
	assert(coordinate.y < shape.y);
	assert(coordinate.z < shape.z);

	return coordinate.x + coordinate.y * shape.x +
	       coordinate.z * shape.x * shape.y;
}

int3 Array3d::coordinate(int index) const
{
	assert(index >= 0);
	assert(index < size());

	return {index % shape.x, (index / shape.x) % shape.y,
	        index / (shape.x * shape.y)};
}

// Provides indexing services into a strictly lower triangular matrix.
// Indexing maps to continious and memory compact space, where each element
// can represent an edge in a fully connected graph of nodes. Number of nodes
// are based on the number of elements in a given 3D grid. Using this type
// the caller can allocate an array of appropriate capacity to hold all edges,
// and index in and out of the array using XYZ coordinates pairs (grid node
// indices).
struct FullyConnectedGraph
{
	struct CoordinatePair
	{
		int3 a;
		int3 b;
	};

	OQMC_HOST_DEVICE FullyConnectedGraph(int3 shape);
	OQMC_HOST_DEVICE long int size() const;
	OQMC_HOST_DEVICE long int index(CoordinatePair coordinates) const;
	OQMC_HOST_DEVICE CoordinatePair coordinates(long int index) const;

	const Array3d array;
};

FullyConnectedGraph::FullyConnectedGraph(int3 shape) : array(shape)
{
}

long int FullyConnectedGraph::size() const
{
	return static_cast<long int>(array.size()) * (array.size() - 1) / 2;
}

long int FullyConnectedGraph::index(CoordinatePair coordinates) const
{
	int i = array.index(coordinates.a);
	int j = array.index(coordinates.b);

	assert(i != j);

	if(i > j)
	{
		swap(i, j);
	}

	assert(i < j);

	return i + FullyConnectedGraph({j, 1, 1}).size();
}

FullyConnectedGraph::CoordinatePair
FullyConnectedGraph::coordinates(long int index) const
{
	assert(index >= 0);
	assert(index < size());

	const int integerTerm = std::sqrt(static_cast<double>(8 * index + 1)) + 1;
	const int subGraphSize = integerTerm / 2;

	const int i = index - FullyConnectedGraph({subGraphSize, 1, 1}).size();
	const int j = subGraphSize;

	return {array.coordinate(i), array.coordinate(j)};
}

void initialiseIndices(Array3d pixelFrame, int* indices)
{
	for(int i = 0; i < pixelFrame.size(); ++i)
	{
		indices[i] = i;
	}
}

void initialiseSwaps(Array3d pixelFrame, bool* swaps)
{
	for(int i = 0; i < pixelFrame.size(); ++i)
	{
		swaps[i] = false;
	}
}

void initialisePermutations(int& seed, Array3d pixelFrame, int* permutations)
{
	auto state = oqmc::pcg::init(seed++);

	for(int i = 0; i < pixelFrame.size(); ++i)
	{
		permutations[i] = i;
	}

	for(int i = 0; i < pixelFrame.size(); ++i)
	{
		swap(permutations[i],
		     permutations[oqmc::pcg::rngBounded(i, pixelFrame.size(), state)]);
	}
}

void initialiseKeys(int& seed, Array3d pixelFrame, std::uint32_t* keys)
{
	auto state = oqmc::pcg::init(seed++);

	for(int i = 0; i < pixelFrame.size(); ++i)
	{
		keys[i] = oqmc::pcg::rng(state);
	}
}

void initialiseRanks(Array3d pixelFrame, std::uint32_t* ranks)
{
	for(int i = 0; i < pixelFrame.size(); ++i)
	{
		ranks[i] = 0;
	}
}

template <typename Sampler, typename Shape>
OQMC_HOST_DEVICE float estimate(const void* cache, std::uint32_t key,
                                std::uint32_t rank, Shape shape, int nsamples)
{
	assert(nsamples >= 0);

	float mean = 0;
	for(int i = 0; i < nsamples; ++i)
	{
		std::uint32_t out[] = {0, 0};
		Sampler::sample(i ^ rank, key, cache, out);

		const auto x = oqmc::uintToFloat(out[0]);
		const auto y = oqmc::uintToFloat(out[1]);
		const auto delta = shape.evaluate(x, y) - mean;
		const auto deltaOverN = delta / (i + 1);

		mean += deltaOverN;
	}

	return mean;
}

template <typename Sampler>
void initialiseEstimates(int nsamples, const void* cache, Array3d pixelFrame,
                         const std::uint32_t* keys, const std::uint32_t* ranks,
                         float* estimates)
{
	auto start = oqmc_progress_start("Computing estimates:", pixelFrame.size());

	for(int index = 0; index < pixelFrame.size(); index += transactionSize)
	{
		const auto func = [=] OQMC_HOST_DEVICE(std::size_t idx) {
			const auto key = keys[idx];
			const auto rank = ranks[idx];
			const auto shape = QuarterGaussian();
			estimates[idx] =
			    estimate<Sampler>(cache, key, rank, shape, nsamples);
		};

		const auto begin = index;
		const auto end =
		    std::min<int>(index + transactionSize, pixelFrame.size());

		OQMC_FORLOOP(func, begin, end);

		oqmc_progress_add("Computing estimates:", pixelFrame.size(), end,
		                  start);
	}

	oqmc_progress_end();
}

template <typename Sampler>
void initialiseErrors(int nsamples, const void* cache, Array3d errorFrame,
                      const std::uint32_t* keys, float* errors)
{
	OrientedHeaviside* heavisides;
	OQMC_ALLOCATE(&heavisides, errorFrame.shape.y);

	OrientedHeaviside::build(errorFrame.shape.y, heavisides);

	auto start = oqmc_progress_start("Computing errors:", errorFrame.size());

	for(int index = 0; index < errorFrame.size(); index += transactionSize)
	{
		const auto func = [=] OQMC_HOST_DEVICE(std::size_t idx) {
			const auto key = keys[errorFrame.coordinate(idx).x];
			const auto rank = 0;
			const auto test = heavisides[errorFrame.coordinate(idx).y];
			errors[idx] = estimate<Sampler>(cache, key, rank, test, nsamples);
		};

		const auto begin = index;
		const auto end =
		    std::min<int>(index + transactionSize, errorFrame.size());

		OQMC_FORLOOP(func, begin, end);

		oqmc_progress_add("Computing errors:", errorFrame.size(), end, start);
	}

	oqmc_progress_end();

	OQMC_FREE(heavisides);
}

template <typename Sampler, bool FlipBit>
void initialiseErrors(int nsamples, const void* cache, Array3d errorFrame,
                      const std::uint32_t* keys, const std::uint32_t* ranks,
                      float* errors)
{
	OrientedHeaviside* heavisides;
	OQMC_ALLOCATE(&heavisides, errorFrame.shape.y);
	OrientedHeaviside::build(errorFrame.shape.y, heavisides);

	int bit;
	if(FlipBit)
	{
		bit = nsamples;
	}
	else
	{
		bit = 0;
	}

	auto start = oqmc_progress_start("Computing errors:", errorFrame.size());

	for(int index = 0; index < errorFrame.size(); index += transactionSize)
	{
		const auto func = [=] OQMC_HOST_DEVICE(std::size_t idx) {
			const auto key = keys[errorFrame.coordinate(idx).x];
			const auto rank = ranks[errorFrame.coordinate(idx).x] ^ bit;
			const auto test = heavisides[errorFrame.coordinate(idx).y];
			errors[idx] = estimate<Sampler>(cache, key, rank, test, nsamples);
		};

		const auto begin = index;
		const auto end =
		    std::min<int>(index + transactionSize, errorFrame.size());

		OQMC_FORLOOP(func, begin, end);

		oqmc_progress_add("Computing errors:", errorFrame.size(), end, start);
	}

	oqmc_progress_end();

	OQMC_FREE(heavisides);
}

OQMC_HOST_DEVICE float squaredDistance(int offsetA, int offsetB, int3 size,
                                       const float* errorsA,
                                       const float* errorsB)
{
	assert(size.x >= 0);
	assert(size.y >= 0);

	float sum = 0;
	float error = 0;
	for(int i = 0; i < size.y; ++i)
	{
		const auto step = i * size.x;

		const auto distance = errorsA[offsetA + step] - errorsB[offsetB + step];
		const auto squared = distance * distance;

		const auto y = squared - error;
		const auto t = sum + y;

		error = (t - sum) - y;
		sum = t;
	}

	return sum;
}

void initialiseDistances(Array3d pixelFrame, Array3d errorFrame,
                         FullyConnectedGraph graphFrame, const float* errors,
                         float* distances)
{
	auto start = oqmc_progress_start("Computing distances:", graphFrame.size());

	for(long int index = 0; index < graphFrame.size(); index += transactionSize)
	{
		const auto func = [=] OQMC_HOST_DEVICE(std::size_t idx) {
			const auto coordinates = graphFrame.coordinates(idx);
			const auto i = pixelFrame.index(coordinates.a);
			const auto j = pixelFrame.index(coordinates.b);
			const auto p = errorFrame.index({i, 0, 0});
			const auto q = errorFrame.index({j, 0, 0});
			distances[idx] =
			    squaredDistance(p, q, errorFrame.shape, errors, errors);
		};

		const auto begin = index;
		const auto end =
		    std::min<long int>(index + transactionSize, graphFrame.size());

		OQMC_FORLOOP(func, begin, end);

		oqmc_progress_add("Computing distances:", graphFrame.size(), end,
		                  start);
	}

	oqmc_progress_end();
}

template <bool CrossErrors>
void initialiseDistances(Array3d pixelFrame, Array3d errorFrame,
                         FullyConnectedGraph graphFrame,
                         const float* errorsHold, const float* errorsSwap,
                         float* distances)
{
	auto start = oqmc_progress_start("Computing distances:", graphFrame.size());

	const float* errorsA;
	const float* errorsB;
	const float* errorsC;
	const float* errorsD;

	if(CrossErrors)
	{
		// Note the specific selection for A, B, C, D here is very important.
		// For example, swaping A and B assignements would introduce an bug.
		errorsA = errorsHold;
		errorsB = errorsSwap;
		errorsC = errorsSwap;
		errorsD = errorsHold;
	}
	else
	{
		errorsA = errorsHold;
		errorsB = errorsHold;
		errorsC = errorsSwap;
		errorsD = errorsSwap;
	}

	for(long int index = 0; index < graphFrame.size(); index += transactionSize)
	{
		const auto func = [=] OQMC_HOST_DEVICE(std::size_t idx) {
			const auto coordinates = graphFrame.coordinates(idx);
			const auto i = pixelFrame.index(coordinates.a);
			const auto j = pixelFrame.index(coordinates.b);
			const auto p = errorFrame.index({i, 0, 0});
			const auto q = errorFrame.index({j, 0, 0});
			distances[idx] =
			    squaredDistance(p, q, errorFrame.shape, errorsA, errorsB) +
			    squaredDistance(p, q, errorFrame.shape, errorsC, errorsD);
		};

		const auto begin = index;
		const auto end =
		    std::min<long int>(index + transactionSize, graphFrame.size());

		OQMC_FORLOOP(func, begin, end);

		oqmc_progress_add("Computing distances:", graphFrame.size(), end,
		                  start);
	}

	oqmc_progress_end();
}

template <bool SwapPixels>
OQMC_HOST_DEVICE float
keysEnergySpatial(Array3d pixelFrame, FullyConnectedGraph graphFrame,
                  int3 pCoordinate, int3 swapCoordinate, const int* indicesA,
                  const float* distances)
{
	constexpr auto sigma = 2.1f;
	constexpr auto sigmaSqr = sigma * sigma;
	constexpr auto sigmaSqrRcp = 1 / sigmaSqr;
	constexpr auto sigmaSqrRcpNeg = -sigmaSqrRcp;

	constexpr auto width = 6;
	constexpr auto min = -width;
	constexpr auto max = +width;

	auto sum = 0.0f;
	for(int j = min; j < max + 1; ++j)
	{
		for(int i = min; i < max + 1; ++i)
		{
			const int3 qCoordinate = {
			    (pCoordinate.x + i) & bitMask(pixelFrame.shape.x),
			    (pCoordinate.y + j) & bitMask(pixelFrame.shape.y),
			    pCoordinate.z,
			};

			if(pCoordinate == qCoordinate)
			{
				continue;
			}

			int pIndex;
			int qIndex;
			if(SwapPixels)
			{
				if(qCoordinate == swapCoordinate)
				{
					pIndex = indicesA[pixelFrame.index(swapCoordinate)];
					qIndex = indicesA[pixelFrame.index(pCoordinate)];
				}
				else
				{
					pIndex = indicesA[pixelFrame.index(swapCoordinate)];
					qIndex = indicesA[pixelFrame.index(qCoordinate)];
				}
			}
			else
			{
				pIndex = indicesA[pixelFrame.index(pCoordinate)];
				qIndex = indicesA[pixelFrame.index(qCoordinate)];
			}

			const FullyConnectedGraph::CoordinatePair pair = {
			    pixelFrame.coordinate(pIndex),
			    pixelFrame.coordinate(qIndex),
			};

			// Note that distances are already squared.
			const auto sampleDistance = distances[graphFrame.index(pair)];
			const auto pixelDistance =
			    std::exp((i * i + j * j) * sigmaSqrRcpNeg);

			sum += sampleDistance * pixelDistance;
		}
	}

	return sum;
}

template <bool SwapPixels>
OQMC_HOST_DEVICE float
keysEnergyTemporal(Array3d pixelFrame, FullyConnectedGraph graphFrame,
                   int3 pCoordinate, int3 swapCoordinate, const int* indicesA,
                   const float* distances)
{
	constexpr auto sigma = 1.5f;
	constexpr auto sigmaSqr = sigma * sigma;
	constexpr auto sigmaSqrRcp = 1 / sigmaSqr;
	constexpr auto sigmaSqrRcpNeg = -sigmaSqrRcp;

	constexpr auto width = 6;
	constexpr auto min = -width;
	constexpr auto max = +width;

	auto sum = 0.0f;
	for(int i = min; i < max + 1; ++i)
	{
		const int3 qCoordinate = {
		    pCoordinate.x,
		    pCoordinate.y,
		    (pCoordinate.z + i) & bitMask(pixelFrame.shape.z),
		};

		if(pCoordinate == qCoordinate)
		{
			continue;
		}

		int pIndex;
		int qIndex;
		if(SwapPixels)
		{
			if(qCoordinate == swapCoordinate)
			{
				pIndex = indicesA[pixelFrame.index(swapCoordinate)];
				qIndex = indicesA[pixelFrame.index(pCoordinate)];
			}
			else
			{
				pIndex = indicesA[pixelFrame.index(swapCoordinate)];
				qIndex = indicesA[pixelFrame.index(qCoordinate)];
			}
		}
		else
		{
			pIndex = indicesA[pixelFrame.index(pCoordinate)];
			qIndex = indicesA[pixelFrame.index(qCoordinate)];
		}

		const FullyConnectedGraph::CoordinatePair pair = {
		    pixelFrame.coordinate(pIndex),
		    pixelFrame.coordinate(qIndex),
		};

		// Note that distances are already squared.
		const auto sampleDistance = distances[graphFrame.index(pair)];
		const auto pixelDistance = std::exp((i * i) * sigmaSqrRcpNeg);

		sum += sampleDistance * pixelDistance;
	}

	return sum;
}

template <bool SwapPixels>
OQMC_HOST_DEVICE float
keysEnergy(Array3d pixelFrame, FullyConnectedGraph graphFrame, int3 pCoordinate,
           int3 swapCoordinate, const int* indicesA, const float* distances)
{
	return keysEnergySpatial<SwapPixels>(pixelFrame, graphFrame, pCoordinate,
	                                     swapCoordinate, indicesA, distances) +
	       keysEnergyTemporal<SwapPixels>(pixelFrame, graphFrame, pCoordinate,
	                                      swapCoordinate, indicesA, distances);
}

void keysOptimise(int niterations, int& seed, Array3d pixelFrame,
                  FullyConnectedGraph graphFrame, const int* permutations,
                  const float* distances, int* indicesA, int* indicesB,
                  std::uint32_t* keys)
{
	auto start = oqmc_progress_start("Optimising keys:", niterations);
	auto state = oqmc::pcg::init(seed++);

	for(int i = 0; i < niterations; ++i)
	{
		const auto rnd = oqmc::pcg::rng(state) & bitMask(pixelFrame.size());

		const auto func = [=] OQMC_HOST_DEVICE(std::size_t idx) {
			const auto aIndex = permutations[idx * 2 + 0] ^ rnd;
			const auto bIndex = permutations[idx * 2 + 1] ^ rnd;
			const auto aCoordinate = pixelFrame.coordinate(aIndex);
			const auto bCoordinate = pixelFrame.coordinate(bIndex);

			const auto last =
			    keysEnergy<false>(pixelFrame, graphFrame, aCoordinate,
			                      bCoordinate, indicesA, distances) +
			    keysEnergy<false>(pixelFrame, graphFrame, bCoordinate,
			                      aCoordinate, indicesA, distances);

			const auto next =
			    keysEnergy<true>(pixelFrame, graphFrame, aCoordinate,
			                     bCoordinate, indicesA, distances) +
			    keysEnergy<true>(pixelFrame, graphFrame, bCoordinate,
			                     aCoordinate, indicesA, distances);

			if(next > last)
			{
				swap(indicesB[aIndex], indicesB[bIndex]);
				swap(keys[aIndex], keys[bIndex]);
			}
		};

		const auto begin = 0;
		const auto end = pixelFrame.size() / 2 /*pairs*/ / 4 /*quarter image*/;

		OQMC_FORLOOP(func, begin, end);
		OQMC_MEMCPY(indicesA, indicesB, sizeof(int) * pixelFrame.size());

		oqmc_progress_add("Optimising keys:", niterations, i + 1, start);
	}

	oqmc_progress_end();
}

template <typename Sampler>
void keysRun(int niterations, int nsamples, int& seed, const void* cache,
             Array3d pixelFrame, Array3d errorFrame,
             FullyConnectedGraph graphFrame, std::uint32_t* keys)
{
	int* indicesA;
	OQMC_ALLOCATE(&indicesA, pixelFrame.size());

	int* indicesB;
	OQMC_ALLOCATE(&indicesB, pixelFrame.size());

	int* permutations;
	OQMC_ALLOCATE(&permutations, pixelFrame.size());

	float* errors;
	OQMC_ALLOCATE(&errors, errorFrame.size());

	float* distances;
	OQMC_ALLOCATE(&distances, graphFrame.size());

	// Precompute the distances between all pixels and cache the result, this
	// prevents the optimiser from needing to compute the distances on the fly
	// for each iteration.
	initialiseIndices(pixelFrame, indicesA);
	initialiseIndices(pixelFrame, indicesB);
	initialisePermutations(seed, pixelFrame, permutations);
	initialiseKeys(seed, pixelFrame, keys);
	initialiseErrors<Sampler>(nsamples, cache, errorFrame, keys, errors);
	initialiseDistances(pixelFrame, errorFrame, graphFrame, errors, distances);

	keysOptimise(niterations, seed, pixelFrame, graphFrame, permutations,
	             distances, indicesA, indicesB, keys);

	OQMC_FREE(indicesA);
	OQMC_FREE(indicesB);
	OQMC_FREE(permutations);
	OQMC_FREE(errors);
	OQMC_FREE(distances);
}

template <bool SwapOrder>
OQMC_HOST_DEVICE float
ranksEnergySpatial(Array3d pixelFrame, FullyConnectedGraph graphFrame,
                   int3 pCoordinate, const bool* swapsA,
                   const float* distancesHold, const float* distancesSwap)
{
	constexpr auto sigma = 2.1f;
	constexpr auto sigmaSqr = sigma * sigma;
	constexpr auto sigmaSqrRcp = 1 / sigmaSqr;
	constexpr auto sigmaSqrRcpNeg = -sigmaSqrRcp;

	constexpr auto width = 6;
	constexpr auto min = -width;
	constexpr auto max = +width;

	auto sum = 0.0f;
	for(int j = min; j < max + 1; ++j)
	{
		for(int i = min; i < max + 1; ++i)
		{
			const int3 qCoordinate = {
			    (pCoordinate.x + i) & bitMask(pixelFrame.shape.x),
			    (pCoordinate.y + j) & bitMask(pixelFrame.shape.y),
			    pCoordinate.z,
			};

			if(pCoordinate == qCoordinate)
			{
				continue;
			}

			const bool pSwap = swapsA[pixelFrame.index(pCoordinate)];
			const bool qSwap = swapsA[pixelFrame.index(qCoordinate)];

			const float* distances;
			if((pSwap == qSwap) != SwapOrder)
			{
				distances = distancesHold;
			}
			else
			{
				distances = distancesSwap;
			}

			const FullyConnectedGraph::CoordinatePair pair = {
			    pCoordinate,
			    qCoordinate,
			};

			// Note that distances are already squared.
			const auto sampleDistance = distances[graphFrame.index(pair)];
			const auto pixelDistance =
			    std::exp((i * i + j * j) * sigmaSqrRcpNeg);

			sum += sampleDistance * pixelDistance;
		}
	}

	return sum;
}

template <bool SwapOrder>
OQMC_HOST_DEVICE float
ranksEnergyTemporal(Array3d pixelFrame, FullyConnectedGraph graphFrame,
                    int3 pCoordinate, const bool* swapsA,
                    const float* distancesHold, const float* distancesSwap)
{
	constexpr auto sigma = 1.5f;
	constexpr auto sigmaSqr = sigma * sigma;
	constexpr auto sigmaSqrRcp = 1 / sigmaSqr;
	constexpr auto sigmaSqrRcpNeg = -sigmaSqrRcp;

	constexpr auto width = 6;
	constexpr auto min = -width;
	constexpr auto max = +width;

	auto sum = 0.0f;
	for(int i = min; i < max + 1; ++i)
	{
		const int3 qCoordinate = {
		    pCoordinate.x,
		    pCoordinate.y,
		    (pCoordinate.z + i) & bitMask(pixelFrame.shape.z),
		};

		if(pCoordinate == qCoordinate)
		{
			continue;
		}

		const bool pSwap = swapsA[pixelFrame.index(pCoordinate)];
		const bool qSwap = swapsA[pixelFrame.index(qCoordinate)];

		const float* distances;
		if((pSwap == qSwap) != SwapOrder)
		{
			distances = distancesHold;
		}
		else
		{
			distances = distancesSwap;
		}

		const FullyConnectedGraph::CoordinatePair pair = {
		    pCoordinate,
		    qCoordinate,
		};

		// Note that distances are already squared.
		const auto sampleDistance = distances[graphFrame.index(pair)];
		const auto pixelDistance = std::exp((i * i) * sigmaSqrRcpNeg);

		sum += sampleDistance * pixelDistance;
	}

	return sum;
}

template <bool SwapOrder>
OQMC_HOST_DEVICE float
ranksEnergy(Array3d pixelFrame, FullyConnectedGraph graphFrame,
            int3 pCoordinate, const bool* swapsA, const float* distancesHold,
            const float* distancesSwap)
{
	return ranksEnergySpatial<SwapOrder>(pixelFrame, graphFrame, pCoordinate,
	                                     swapsA, distancesHold, distancesSwap) +
	       ranksEnergyTemporal<SwapOrder>(pixelFrame, graphFrame, pCoordinate,
	                                      swapsA, distancesHold, distancesSwap);
}

void ranksOptimise(int niterations, int& seed, Array3d pixelFrame,
                   FullyConnectedGraph graphFrame, const int* permutations,
                   const float* distancesHold, const float* distancesSwap,
                   bool* swapsA, bool* swapsB)
{
	auto start = oqmc_progress_start("Optimising ranks:", niterations);
	auto state = oqmc::pcg::init(seed++);

	for(int i = 0; i < niterations; ++i)
	{
		const auto rnd = oqmc::pcg::rng(state) & bitMask(pixelFrame.size());

		const auto func = [=] OQMC_HOST_DEVICE(std::size_t idx) {
			const auto index = permutations[idx] ^ rnd;
			const auto coordinate = pixelFrame.coordinate(index);

			const auto last =
			    ranksEnergy<false>(pixelFrame, graphFrame, coordinate, swapsA,
			                       distancesHold, distancesSwap);

			const auto next =
			    ranksEnergy<true>(pixelFrame, graphFrame, coordinate, swapsA,
			                      distancesHold, distancesSwap);

			if(next > last)
			{
				swapsB[index] = !swapsB[index];
			}
		};

		const auto begin = 0;
		const auto end = pixelFrame.size() / 4 /*quarter image*/;

		OQMC_FORLOOP(func, begin, end);
		OQMC_MEMCPY(swapsA, swapsB, sizeof(bool) * pixelFrame.size());

		oqmc_progress_add("Optimising ranks:", niterations, i + 1, start);
	}

	oqmc_progress_end();
}

template <typename Sampler>
void ranksRun(int niterations, int nsamples, int& seed, const void* cache,
              Array3d pixelFrame, Array3d errorFrame,
              FullyConnectedGraph graphFrame, const std::uint32_t* keys,
              std::uint32_t* ranks)
{
	bool* swapsA;
	OQMC_ALLOCATE(&swapsA, pixelFrame.size());

	bool* swapsB;
	OQMC_ALLOCATE(&swapsB, pixelFrame.size());

	int* permutations;
	OQMC_ALLOCATE(&permutations, pixelFrame.size());

	float* errorsHold;
	OQMC_ALLOCATE(&errorsHold, errorFrame.size());

	float* errorsSwap;
	OQMC_ALLOCATE(&errorsSwap, errorFrame.size());

	float* distancesHold;
	OQMC_ALLOCATE(&distancesHold, graphFrame.size());

	float* distancesSwap;
	OQMC_ALLOCATE(&distancesSwap, graphFrame.size());

	initialisePermutations(seed, pixelFrame, permutations);
	initialiseRanks(pixelFrame, ranks);

	// Iterate over all power-of-two sample counts.
	for(int i = nsamples >> 1; i > 0; i = i >> 1)
	{
		fprintf(stderr, "Processing ranks for %i samples.\n", i);

		initialiseSwaps(pixelFrame, swapsA);
		initialiseSwaps(pixelFrame, swapsB);

		constexpr auto hold = false;
		constexpr auto swap = true;

		initialiseErrors<Sampler, hold>(i, cache, errorFrame, keys, ranks,
		                                errorsHold);
		initialiseErrors<Sampler, swap>(i, cache, errorFrame, keys, ranks,
		                                errorsSwap);

		// Precompute the distances between all pixels and cache the result,
		// this prevents the optimiser from needing to compute the distances on
		// the fly for each iteration. Twice as many connections exist fort rank
		// optimisation as there needs to be a distance for the swaped, and non
		// swaped index ranges.
		initialiseDistances<hold>(pixelFrame, errorFrame, graphFrame,
		                          errorsHold, errorsSwap, distancesHold);
		initialiseDistances<swap>(pixelFrame, errorFrame, graphFrame,
		                          errorsHold, errorsSwap, distancesSwap);

		ranksOptimise(niterations, seed, pixelFrame, graphFrame, permutations,
		              distancesHold, distancesSwap, swapsA, swapsB);

		for(int j = 0; j < pixelFrame.size(); ++j)
		{
			if(swapsA[j])
			{
				ranks[j] ^= i;
			}
			else
			{
				ranks[j] ^= 0;
			}
		}
	}

	OQMC_FREE(swapsA);
	OQMC_FREE(swapsB);
	OQMC_FREE(permutations);
	OQMC_FREE(errorsHold);
	OQMC_FREE(errorsSwap);
	OQMC_FREE(distancesHold);
	OQMC_FREE(distancesSwap);
}

struct Output
{
	std::uint32_t* keys;
	std::uint32_t* ranks;
	float* estimates;
	float* frequencies;
};

template <typename Sampler>
void output(int nsamples, const void* cache, Array3d pixelFrame,
            std::uint32_t* keys, std::uint32_t* ranks, Output out)
{
	float* estimates;
	OQMC_ALLOCATE(&estimates, pixelFrame.size());

	initialiseEstimates<Sampler>(nsamples, cache, pixelFrame, keys, ranks,
	                             estimates);

	for(int i = 0; i < pixelFrame.size(); ++i)
	{
		out.keys[i] = keys[i];
		out.ranks[i] = ranks[i];
		out.estimates[i] = estimates[i]; // NOLINT: false positive
	}

	oqmc_frequency_discrete_3d(pixelFrame.shape.x, pixelFrame.shape.z,
	                           out.estimates, out.frequencies);

	OQMC_FREE(estimates);
}

template <typename Sampler>
void run(int ntests, int niterations, int nsamples, int resolution, int depth,
         int seed, Output out)
{
	const auto cache = Sampler::initialiseCache();

	const auto pixelFrame = Array3d({resolution, resolution, depth});
	const auto errorFrame = Array3d({pixelFrame.size(), ntests, 1});
	const auto graphFrame = FullyConnectedGraph(pixelFrame.shape);

	const auto pixelCost = pixelFrame.size() * 4 * 5 / 1000000.f;
	const auto errorCost = errorFrame.size() * 4 * 2 / 1000000.f;
	const auto graphCost = graphFrame.size() * 4 * 2 / 1000000.f;

	fprintf(stderr,
	        "Using %i tests, %i iterations, %i samples, %i resolution, %i depth"
	        "; Memory cost is %.2fMB.\n",
	        ntests, niterations, nsamples, resolution, depth,
	        pixelCost + errorCost + graphCost);

	std::uint32_t* keys;
	OQMC_ALLOCATE(&keys, pixelFrame.size());

	std::uint32_t* ranks;
	OQMC_ALLOCATE(&ranks, pixelFrame.size());

	keysRun<Sampler>(niterations, nsamples, seed, cache, pixelFrame, errorFrame,
	                 graphFrame, keys);

	ranksRun<Sampler>(niterations, nsamples, seed, cache, pixelFrame,
	                  errorFrame, graphFrame, keys, ranks);

	output<Sampler>(nsamples, cache, pixelFrame, keys, ranks, out);

	OQMC_FREE(keys);
	OQMC_FREE(ranks);

	Sampler::deinitialiseCache(cache);
}

struct Pmj
{
	OQMC_HOST_DEVICE static void* initialiseCache()
	{
		std::uint32_t(*samples)[4];
		OQMC_ALLOCATE(&samples, oqmc::State64Bit::maxIndexSize);

		oqmc::stochasticPmjInit(oqmc::State64Bit::maxIndexSize, samples);

		return samples;
	}

	OQMC_HOST_DEVICE static void deinitialiseCache(void* cache)
	{
		OQMC_FREE(cache);
	}

	OQMC_HOST_DEVICE static void sample(int index, std::uint32_t hash,
	                                    const void* cache, std::uint32_t out[2])
	{
		auto samples = static_cast<const std::uint32_t(*)[4]>(cache);
		oqmc::shuffledScrambledLookup<4, 2>(index, hash, samples, out);
	}
};

struct Sobol
{
	OQMC_HOST_DEVICE static void* initialiseCache()
	{
		return nullptr;
	}

	OQMC_HOST_DEVICE static void deinitialiseCache(void* cache)
	{
		OQMC_MAYBE_UNUSED(cache);
	}

	OQMC_HOST_DEVICE static void sample(int index, std::uint32_t hash,
	                                    const void* cache, std::uint32_t out[2])
	{
		OQMC_MAYBE_UNUSED(cache);

		oqmc::shuffledScrambledSobol<2>(index, hash, out);
	}
};

struct Lattice
{
	OQMC_HOST_DEVICE static void* initialiseCache()
	{
		return nullptr;
	}

	OQMC_HOST_DEVICE static void deinitialiseCache(void* cache)
	{
		OQMC_MAYBE_UNUSED(cache);
	}

	OQMC_HOST_DEVICE static void sample(int index, std::uint32_t hash,
	                                    const void* cache, std::uint32_t out[2])
	{
		OQMC_MAYBE_UNUSED(cache);

		oqmc::shuffledRotatedLattice<2>(index, hash, out);
	}
};

} // namespace

// This optimisation process is based on 'Lessons Learned and Improvements
// when Building Screen-Space Samplers with Blue-Noise Error Distribution'
// by Laurent Belcour and Eric Heitz. It goes beyond the implementation
// example from the publication and adds support for optimising ranks
// to allow for progressive sampling. It also extends the method to be
// spatial temporal rather than just spatial. This is done by incorpiating
// ideas from 'Spatiotemporal Blue Noise Masks' by Alan Wolf, et. al. The
// implementation is also generalised, so that as long a base sample pattern
// can be parameterised using a single 32 bit integer for randomisation, it
// does not matter what method used for randomisation. If you would like to
// use this optimiser for your own sampler, all you need to do is include the
// implementation here and make sure it conforms to the interface shown above.
// Note that not all randomisation methods optimise as well as others, for more
// details see the original paper.
OQMC_CABI bool oqmc_optimise(const char* name, int ntests, int niterations,
                             int nsamples, int resolution, int depth, int seed,
                             uint32_t* keys, uint32_t* ranks, float* estimates,
                             float* frequencies)
{
	assert(ntests > 0);
	assert(niterations > 0);
	assert(nsamples > 0);
	assert(isPowerOfTwo(nsamples));
	assert(resolution > 0);
	assert(isPowerOfTwo(resolution));
	assert(depth > 0);
	assert(isPowerOfTwo(depth));
	assert(keys);
	assert(ranks);
	assert(estimates);
	assert(frequencies);

	OQMC_MAYBE_UNUSED(isPowerOfTwo);

	if(std::string(name) == "pmj")
	{
		run<Pmj>(ntests, niterations, nsamples, resolution, depth, seed,
		         {keys, ranks, estimates, frequencies});

		return true;
	}

	if(std::string(name) == "sobol")
	{
		run<Sobol>(ntests, niterations, nsamples, resolution, depth, seed,
		           {keys, ranks, estimates, frequencies});

		return true;
	}

	if(std::string(name) == "lattice")
	{
		run<Lattice>(ntests, niterations, nsamples, resolution, depth, seed,
		             {keys, ranks, estimates, frequencies});

		return true;
	}

	return false;
}
