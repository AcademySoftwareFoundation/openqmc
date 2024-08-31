// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/// @file
/// @details Latticebn sampler implementation.

#pragma once

#include "bntables.h"
#include "gpu.h"
#include "pcg.h"
#include "rank1.h"
#include "sampler.h"
#include "state.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace oqmc
{

/// @cond
class LatticeBnImpl
{
	// See SamplerInterface for public API documentation.
	friend SamplerInterface<LatticeBnImpl>;

	struct CacheType
	{
		std::uint32_t keyTable[bntables::size];
		std::uint32_t rankTable[bntables::size];
	};

	static constexpr std::size_t cacheSize = sizeof(CacheType);
	static void initialiseCache(void* cache);

	/*AUTO_DEFINED*/ LatticeBnImpl() = default;
	OQMC_HOST_DEVICE LatticeBnImpl(State64Bit state, const CacheType* cache);
	OQMC_HOST_DEVICE LatticeBnImpl(int x, int y, int frame, int index,
	                               const void* cache);

	OQMC_HOST_DEVICE LatticeBnImpl newDomain(int key) const;
	OQMC_HOST_DEVICE LatticeBnImpl newDomainSplit(int key, int size,
	                                              int index) const;
	OQMC_HOST_DEVICE LatticeBnImpl newDomainDistrib(int key, int index) const;

	template <int Size>
	OQMC_HOST_DEVICE void drawSample(std::uint32_t sample[Size]) const;

	template <int Size>
	OQMC_HOST_DEVICE void drawRnd(std::uint32_t rnd[Size]) const;

	State64Bit state;
	const CacheType* cache;
};

inline void LatticeBnImpl::initialiseCache(void* cache)
{
	assert(cache);

	auto typedCache = static_cast<CacheType*>(cache);

	std::memcpy(typedCache->keyTable, bntables::lattice::keyTable,
	            sizeof(bntables::lattice::keyTable));
	std::memcpy(typedCache->rankTable, bntables::lattice::rankTable,
	            sizeof(bntables::lattice::rankTable));
}

inline LatticeBnImpl::LatticeBnImpl(State64Bit state, const CacheType* cache)
    : state(state), cache(cache)
{
	assert(cache);
}

inline LatticeBnImpl::LatticeBnImpl(int x, int y, int frame, int index,
                                    const void* cache)
    : state(x, y, frame, index), cache(static_cast<const CacheType*>(cache))
{
	assert(cache);
}

inline LatticeBnImpl LatticeBnImpl::newDomain(int key) const
{
	return {state.newDomain(key), cache};
}

inline LatticeBnImpl LatticeBnImpl::newDomainSplit(int key, int size,
                                                   int index) const
{
	return {state.newDomainSplit(key, size, index), cache};
}

inline LatticeBnImpl LatticeBnImpl::newDomainDistrib(int key, int index) const
{
	return {state.newDomainDistrib(key, index), cache};
}

template <int Size>
void LatticeBnImpl::drawSample(std::uint32_t sample[Size]) const
{
	constexpr auto xBits = State64Bit::spatialEncodeBitSizeX;
	constexpr auto yBits = State64Bit::spatialEncodeBitSizeY;

	static_assert(xBits == bntables::xBits,
	              "Pixel x encoding must match table.");
	static_assert(yBits == bntables::yBits,
	              "Pixel y encoding must match table.");

	const auto table = bntables::tableValue<xBits, yBits, 0>(
	    state.pixelId, pcg::output(state.patternId), cache->keyTable,
	    cache->rankTable);

	shuffledRotatedLattice<Size>(state.sampleId ^ table.rank, table.key,
	                             sample);
}

template <int Size>
void LatticeBnImpl::drawRnd(std::uint32_t rnd[Size]) const
{
	state.newDomain(state.pixelId).drawRnd<Size>(rnd);
}
/// @endcond

/// Blue noise variant of lattice sampler.
///
/// Same as oqmc::LatticeSampler, with additional spatial blue noise dithering
/// between pixels, with progressive pixel sampling support.
///
/// @ingroup samplers
using LatticeBnSampler = SamplerInterface<LatticeBnImpl>;

} // namespace oqmc
