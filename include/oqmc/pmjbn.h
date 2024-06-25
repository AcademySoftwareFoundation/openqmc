// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/**
 * @file
 * @details Pmjbn sampler implementation.
 */

#pragma once

#include "bntables.h"
#include "gpu.h"
#include "lookup.h"
#include "pcg.h"
#include "sampler.h"
#include "state.h"
#include "stochastic.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace oqmc
{

/// @cond
class PmjBnImpl
{
	// See SamplerInterface for public API documentation.
	friend SamplerInterface<PmjBnImpl>;

	struct CacheType
	{
		std::uint32_t samples[State64Bit::maxIndexSize][4];
		std::uint32_t keyTable[bntables::size];
		std::uint32_t rankTable[bntables::size];
	};

	static constexpr std::size_t cacheSize = sizeof(CacheType);
	static void initialiseCache(void* cache);

	/*AUTO_DEFINED*/ PmjBnImpl() = default;
	OQMC_HOST_DEVICE PmjBnImpl(State64Bit state, const CacheType* cache);
	OQMC_HOST_DEVICE PmjBnImpl(int x, int y, int frame, int index,
	                           const void* cache);

	OQMC_HOST_DEVICE PmjBnImpl newDomain(int key) const;
	OQMC_HOST_DEVICE PmjBnImpl newDomainDistrib(int key, int index) const;
	OQMC_HOST_DEVICE PmjBnImpl newDomainSplit(int key, int size,
	                                          int index) const;

	template <int Size>
	OQMC_HOST_DEVICE void drawSample(std::uint32_t sample[Size]) const;

	template <int Size>
	OQMC_HOST_DEVICE void drawRnd(std::uint32_t rnd[Size]) const;

	State64Bit state;
	const CacheType* cache;
};

inline void PmjBnImpl::initialiseCache(void* cache)
{
	assert(cache);

	auto typedCache = static_cast<CacheType*>(cache);

	stochasticPmjInit(State64Bit::maxIndexSize, typedCache->samples);

	std::memcpy(typedCache->keyTable, bntables::pmj::keyTable,
	            sizeof(bntables::pmj::keyTable));
	std::memcpy(typedCache->rankTable, bntables::pmj::rankTable,
	            sizeof(bntables::pmj::rankTable));
}

inline PmjBnImpl::PmjBnImpl(State64Bit state, const CacheType* cache)
    : state(state), cache(cache)
{
	assert(cache);
}

inline PmjBnImpl::PmjBnImpl(int x, int y, int frame, int index,
                            const void* cache)
    : state(x, y, frame, index), cache(static_cast<const CacheType*>(cache))
{
	assert(cache);
}

inline PmjBnImpl PmjBnImpl::newDomain(int key) const
{
	return {state.newDomain(key), cache};
}

inline PmjBnImpl PmjBnImpl::newDomainDistrib(int key, int index) const
{
	return {state.newDomainDistrib(key, index), cache};
}

inline PmjBnImpl PmjBnImpl::newDomainSplit(int key, int size, int index) const
{
	return {state.newDomainSplit(key, size, index), cache};
}

template <int Size>
void PmjBnImpl::drawSample(std::uint32_t sample[Size]) const
{
	constexpr auto xBits = State64Bit::spatialEncodeBitSizeX;
	constexpr auto yBits = State64Bit::spatialEncodeBitSizeY;
	constexpr auto zBits = State64Bit::temporalEncodeBitSize;

	static_assert(xBits == bntables::xBits,
	              "Pixel x encoding must match table.");
	static_assert(yBits == bntables::yBits,
	              "Pixel y encoding must match table.");
	static_assert(zBits == bntables::zBits,
	              "Pixel z encoding must match table.");

	const auto table = bntables::tableValue<xBits, yBits, zBits>(
	    state.pixelId, pcg::output(state.patternId), cache->keyTable,
	    cache->rankTable);

	shuffledScrambledLookup<4, Size>(state.sampleId ^ table.rank, table.key,
	                                 cache->samples, sample);
}

template <int Size>
void PmjBnImpl::drawRnd(std::uint32_t rnd[Size]) const
{
	state.newDomain(state.pixelId).drawRnd<Size>(rnd);
}
/// @endcond

/**
 * @brief Blue noise variant of pmj sampler.
 * @details Same as oqmc::PmjSampler, with additional spatial temporal blue
 * noise dithering between pixels, with progressive pixel sampling support.
 *
 * @ingroup samplers
 */
using PmjBnSampler = SamplerInterface<PmjBnImpl>;

} // namespace oqmc
