// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/**
 * @file
 * @details Sobolbn sampler implementation.
 */

#pragma once

#include "bntables.h"
#include "gpu.h"
#include "owen.h"
#include "pcg.h"
#include "sampler.h"
#include "state.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace oqmc
{

/**
 * @brief Sobol blue noise sampler implementation without public interface.
 * @details Private implementation details of the sobol blue noise sampler.
 * Use the aliased SobolBnSampler type to access the sampler via the
 * SamplerInterface.
 */
class SobolBnImpl
{
	// See SamplerInterface for public API documentation.
	friend SamplerInterface<SobolBnImpl>;

	struct CacheType
	{
		std::uint32_t keyTable[bntables::size];
		std::uint32_t rankTable[bntables::size];
	};

	static constexpr std::size_t cacheSize = sizeof(CacheType);
	static void initialiseCache(void* cache);

	/*AUTO_DEFINED*/ SobolBnImpl() = default;
	OQMC_HOST_DEVICE SobolBnImpl(State64Bit state, const CacheType* cache);
	OQMC_HOST_DEVICE SobolBnImpl(int x, int y, int frame, int index,
	                             const void* cache);

	OQMC_HOST_DEVICE SobolBnImpl newDomain(int key) const;
	OQMC_HOST_DEVICE SobolBnImpl newDomainDistrib(int key, int index) const;
	OQMC_HOST_DEVICE SobolBnImpl newDomainSplit(int key, int size,
	                                            int index) const;

	template <int Size>
	OQMC_HOST_DEVICE void drawSample(std::uint32_t sample[Size]) const;

	template <int Size>
	OQMC_HOST_DEVICE void drawRnd(std::uint32_t rnd[Size]) const;

	State64Bit state;
	const CacheType* cache;
};

inline void SobolBnImpl::initialiseCache(void* cache)
{
	assert(cache);

	auto typedCache = static_cast<CacheType*>(cache);

	std::memcpy(typedCache->keyTable, bntables::sobol::keyTable,
	            sizeof(bntables::sobol::keyTable));
	std::memcpy(typedCache->rankTable, bntables::sobol::rankTable,
	            sizeof(bntables::sobol::rankTable));
}

inline SobolBnImpl::SobolBnImpl(State64Bit state, const CacheType* cache)
    : state(state), cache(cache)
{
	assert(cache);
}

inline SobolBnImpl::SobolBnImpl(int x, int y, int frame, int index,
                                const void* cache)
    : state(x, y, frame, index), cache(static_cast<const CacheType*>(cache))
{
	assert(cache);
}

inline SobolBnImpl SobolBnImpl::newDomain(int key) const
{
	return {state.newDomain(key), cache};
}

inline SobolBnImpl SobolBnImpl::newDomainDistrib(int key, int index) const
{
	return {state.newDomainDistrib(key, index), cache};
}

inline SobolBnImpl SobolBnImpl::newDomainSplit(int key, int size,
                                               int index) const
{
	return {state.newDomainSplit(key, size, index), cache};
}

template <int Size>
void SobolBnImpl::drawSample(std::uint32_t sample[Size]) const
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

	shuffledScrambledSobol<Size>(state.sampleId ^ table.rank, table.key,
	                             sample);
}

template <int Size>
void SobolBnImpl::drawRnd(std::uint32_t rnd[Size]) const
{
	state.newDomain(state.pixelId).drawRnd<Size>(rnd);
}

/**
 * @brief Blue noise variant of sobol sampler.
 * @details Same as oqmc::SobolSampler, with additional spatial temporal blue
 * noise dithering between pixels, with progressive pixel sampling support.
 *
 * @ingroup samplers
 */
using SobolBnSampler = SamplerInterface<SobolBnImpl>;

} // namespace oqmc
