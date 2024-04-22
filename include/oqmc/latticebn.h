// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/**
 * @file
 * @details Latticebn sampler implementation.
 */

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

/**
 * @brief Lattice blue noise sampler implementation without public interface.
 * @details Private implementation details of the lattice blue noise sampler.
 * Use the aliased LatticeBnSampler type below to access the sampler via the
 * SamplerInterface.
 *
 * Same as oqmc::LatticeImpl, with additional spatial blue noise dithering
 * between pixels, with progressive ranking for progressive pixel sampling.
 * This involves an offline optimisation process that's based on the work by
 * Belcour and Heitz in 'Lessons Learned and Improvements when Building Screen-
 * Space Samplers with Blue-Noise Error Distribution', and extends temporally
 * as described by Wolfe et al. in 'Spatiotemporal Blue Noise Masks'.
 *
 * The sampler achieves a blue noise distribution using two pixel tables, one
 * holds keys to seed the sequence, and the other index ranks. These tables are
 * then randomised by toroidally shifting the table lookups for each domain
 * using random offsets. Correlation between the offsets and the pixels allows
 * for a single pair of tables to provide keys and ranks for many domains.
 *
 * Although the spatial temporal blue noise doesn't reduce the error for an
 * individual pixel, it does give a better perceptual result due to less low
 * frequency structure in the error between pixels. Also if an image is either
 * spatially or temporally filtered (as with denoising or temporal
 * anti-aliasing), the resulting error can be much lower when using a blue noise
 * variant.
 *
 * The blue noise variant is recommended over the base version of the sampler,
 * as the additional performance cost will be minimal in relation to the gains
 * to be had at low sample counts. However the access to the data tables could
 * have a larger impact on performance depending on the artchecture (GPU), so
 * it is worth benchmarking if this is a concern.
 */
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
	OQMC_HOST_DEVICE LatticeBnImpl newDomainDistrib(int key, int index) const;
	OQMC_HOST_DEVICE LatticeBnImpl newDomainSplit(int key, int size,
	                                              int index) const;

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

inline LatticeBnImpl LatticeBnImpl::newDomainDistrib(int key, int index) const
{
	return {state.newDomainDistrib(key, index), cache};
}

inline LatticeBnImpl LatticeBnImpl::newDomainSplit(int key, int size,
                                                   int index) const
{
	return {state.newDomainSplit(key, size, index), cache};
}

template <int Size>
void LatticeBnImpl::drawSample(std::uint32_t sample[Size]) const
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

	shuffledRotatedLattice<Size>(state.sampleId ^ table.rank, table.key,
	                             sample);
}

template <int Size>
void LatticeBnImpl::drawRnd(std::uint32_t rnd[Size]) const
{
	state.newDomain(state.pixelId).drawRnd<Size>(rnd);
}

using LatticeBnSampler = SamplerInterface<LatticeBnImpl>;

} // namespace oqmc
