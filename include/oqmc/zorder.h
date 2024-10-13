// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/// @file
/// @details Zorder sampler implementation.

#pragma once

#include "bntables.h"
#include "gpu.h"
#include "owen.h"
#include "sampler.h"
#include "state.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace oqmc
{

/// @cond
class ZorderImpl
{
	// See SamplerInterface for public API documentation.
	friend SamplerInterface<ZorderImpl>;

	struct CacheType
	{
		std::uint32_t indexTable[bntables::size];
	};

	static constexpr std::size_t cacheSize = sizeof(CacheType);
	static void initialiseCache(void* cache);

	/*AUTO_DEFINED*/ ZorderImpl() = default;
	OQMC_HOST_DEVICE ZorderImpl(State64Bit state, const CacheType* cache);
	OQMC_HOST_DEVICE ZorderImpl(int x, int y, int frame, int index,
	                            const void* cache);

	OQMC_HOST_DEVICE ZorderImpl newDomain(int key) const;
	OQMC_HOST_DEVICE ZorderImpl newDomainSplit(int key, int size,
	                                           int index) const;
	OQMC_HOST_DEVICE ZorderImpl newDomainDistrib(int key, int index) const;

	template <int Size>
	OQMC_HOST_DEVICE void drawSample(std::uint32_t sample[Size]) const;

	template <int Size>
	OQMC_HOST_DEVICE void drawRnd(std::uint32_t rnd[Size]) const;

	State64Bit state;
	const CacheType* cache;
};

inline void ZorderImpl::initialiseCache(void* cache)
{
	assert(cache);

	auto typedCache = static_cast<CacheType*>(cache);

	std::memcpy(typedCache->indexTable, bntables::hilbert::indexTable,
	            sizeof(bntables::hilbert::indexTable));
}

inline ZorderImpl::ZorderImpl(State64Bit state, const CacheType* cache)
    : state(state), cache(cache)
{
	assert(cache);
}

inline ZorderImpl::ZorderImpl(int x, int y, int frame, int index,
                              const void* cache)
    : state(x, y, frame, index), cache(static_cast<const CacheType*>(cache))
{
	assert(cache);
}

inline ZorderImpl ZorderImpl::newDomain(int key) const
{
	return {state.newDomain(key), cache};
}

inline ZorderImpl ZorderImpl::newDomainSplit(int key, int size, int index) const
{
	return {state.newDomainSplit(key, size, index), cache};
}

inline ZorderImpl ZorderImpl::newDomainDistrib(int key, int index) const
{
	return {state.newDomainDistrib(key, index), cache};
}

template <int Size>
void ZorderImpl::drawSample(std::uint32_t sample[Size]) const
{
	constexpr auto sampleCount = 1;

	const auto hilbert = cache->indexTable[state.pixelId];
	const auto sampleId = hilbert * sampleCount + state.sampleId;

	shuffledScrambledSobol<Size>(sampleId, pcg::output(state.patternId),
	                             sample);
}

template <int Size>
void ZorderImpl::drawRnd(std::uint32_t rnd[Size]) const
{
	state.drawRnd<Size>(rnd);
}
/// @endcond

/// Z ordered sobol sampler.
///
/// TODO: write up description.
///
/// @ingroup samplers
using ZorderSampler = SamplerInterface<ZorderImpl>;

} // namespace oqmc
