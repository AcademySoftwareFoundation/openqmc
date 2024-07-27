// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/// @file
/// @details Pmj sampler implementation.

#pragma once

#include "gpu.h"
#include "lookup.h"
#include "sampler.h"
#include "state.h"
#include "stochastic.h"

#include <cstddef>
#include <cstdint>

namespace oqmc
{

/// @cond
class PmjImpl
{
	// See SamplerInterface for public API documentation.
	friend SamplerInterface<PmjImpl>;

	struct CacheType
	{
		std::uint32_t samples[State64Bit::maxIndexSize][4];
	};

	static constexpr std::size_t cacheSize = sizeof(CacheType);
	static void initialiseCache(void* cache);

	/*AUTO_DEFINED*/ PmjImpl() = default;
	OQMC_HOST_DEVICE PmjImpl(State64Bit state, const CacheType* cache);
	OQMC_HOST_DEVICE PmjImpl(int x, int y, int frame, int index,
	                         const void* cache);

	OQMC_HOST_DEVICE PmjImpl newDomain(int key) const;
	OQMC_HOST_DEVICE PmjImpl newDomainSplit(int key, int size, int index) const;
	OQMC_HOST_DEVICE PmjImpl newDomainDistrib(int key, int index) const;

	template <int Size>
	OQMC_HOST_DEVICE void drawSample(std::uint32_t sample[Size]) const;

	template <int Size>
	OQMC_HOST_DEVICE void drawRnd(std::uint32_t rnd[Size]) const;

	State64Bit state;
	const CacheType* cache;
};

inline void PmjImpl::initialiseCache(void* cache)
{
	assert(cache);

	auto typedCache = static_cast<CacheType*>(cache);

	stochasticPmjInit(State64Bit::maxIndexSize, typedCache->samples);
}

inline PmjImpl::PmjImpl(State64Bit state, const CacheType* cache)
    : state(state), cache(cache)
{
	assert(cache);
}

inline PmjImpl::PmjImpl(int x, int y, int frame, int index, const void* cache)
    : state(x, y, frame, index), cache(static_cast<const CacheType*>(cache))
{
	assert(cache);

	state = state.pixelDecorrelate();
}

inline PmjImpl PmjImpl::newDomain(int key) const
{
	return {state.newDomain(key), cache};
}

inline PmjImpl PmjImpl::newDomainSplit(int key, int size, int index) const
{
	return {state.newDomainSplit(key, size, index), cache};
}

inline PmjImpl PmjImpl::newDomainDistrib(int key, int index) const
{
	return {state.newDomainDistrib(key, index), cache};
}

template <int Size>
void PmjImpl::drawSample(std::uint32_t sample[Size]) const
{
	shuffledScrambledLookup<4, Size>(
	    state.sampleId, pcg::output(state.patternId), cache->samples, sample);
}

template <int Size>
void PmjImpl::drawRnd(std::uint32_t rnd[Size]) const
{
	state.drawRnd<Size>(rnd);
}
/// @endcond

/// Low discrepancy pmj sampler.
///
/// The implementation uses the stochastic method described by Helmer et la. in
/// 'Stochastic Generation of (t, s) Sample Sequences' to efficiently construct
/// a progressive multi-jittered (0,2) sequence. The first pair of dimensions in
/// a domain have the same intergration properties as the Sobol implementation.
/// However as the sequence doesn't extend to more than two dimensions, the
/// second pair is randomised relative to the first in a single domain.
///
/// This sampler pre-computes a base 4D pattern for all sample indices during
/// the cache initialisation. Permuted index values are then looked up from
/// memory at runtime, before being XOR scrambled. This amortises the cost of
/// initialisation. The rate of integration is very high, especially for the
/// first and second pairs of dimensions. You may however not want to use this
/// implementation if memory space or access is a concern.
///
/// @ingroup samplers
using PmjSampler = SamplerInterface<PmjImpl>;

} // namespace oqmc
