// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/// @file
/// @details Lattice sampler implementation.

#pragma once

#include "gpu.h"
#include "rank1.h"
#include "sampler.h"
#include "state.h"
#include "unused.h"

#include <cstddef>
#include <cstdint>

namespace oqmc
{

/// @cond
class LatticeImpl
{
	// See SamplerInterface for public API documentation.
	friend SamplerInterface<LatticeImpl>;

	static constexpr std::size_t cacheSize = 0;
	static void initialiseCache(void* cache);

	/*AUTO_DEFINED*/ LatticeImpl() = default;
	OQMC_HOST_DEVICE LatticeImpl(State64Bit state);
	OQMC_HOST_DEVICE LatticeImpl(int x, int y, int frame, int index,
	                             const void* cache);

	OQMC_HOST_DEVICE LatticeImpl newDomain(int key) const;
	OQMC_HOST_DEVICE LatticeImpl newDomainSplit(int key, int size,
	                                            int index) const;
	OQMC_HOST_DEVICE LatticeImpl newDomainDistrib(int key, int index) const;

	template <int Size>
	OQMC_HOST_DEVICE void drawSample(std::uint32_t sample[Size]) const;

	template <int Size>
	OQMC_HOST_DEVICE void drawRnd(std::uint32_t rnd[Size]) const;

	State64Bit state;
};

inline void LatticeImpl::initialiseCache(void* cache)
{
	OQMC_MAYBE_UNUSED(cache);
}

inline LatticeImpl::LatticeImpl(State64Bit state) : state(state)
{
}

inline LatticeImpl::LatticeImpl(int x, int y, int frame, int index,
                                const void* cache)
    : state(x, y, frame, index)
{
	OQMC_MAYBE_UNUSED(cache);
	state = state.pixelDecorrelate();
}

inline LatticeImpl LatticeImpl::newDomain(int key) const
{
	return {state.newDomain(key)};
}

inline LatticeImpl LatticeImpl::newDomainSplit(int key, int size,
                                               int index) const
{
	return {state.newDomainSplit(key, size, index)};
}

inline LatticeImpl LatticeImpl::newDomainDistrib(int key, int index) const
{
	return {state.newDomainDistrib(key, index)};
}

template <int Size>
void LatticeImpl::drawSample(std::uint32_t sample[Size]) const
{
	shuffledRotatedLattice<Size>(state.sampleId, state.patternId, sample);
}

template <int Size>
void LatticeImpl::drawRnd(std::uint32_t rnd[Size]) const
{
	state.drawRnd<Size>(rnd);
}
/// @endcond

/// Rank one lattice sampler.
/// The implementation uses the generator vector from Hickernell et al. in
/// 'Weighted compound integration rules with higher order convergence for all
/// N' to construct a 4D lattice. This is then made into a progressive sequence
/// using a scalar based on a radical inversion of the sample index.
/// Randomisation uses toroidal shifts.
///
/// This sampler has no cache initialisation cost, it generates all samples on
/// the fly without touching memory. Runtime performance is also high with a
/// relatively low computation cost for a single draw sample call. However the
/// rate of integration per pixel can be lower when compared to other samplers.
///
/// @ingroup samplers
using LatticeSampler = SamplerInterface<LatticeImpl>;

} // namespace oqmc
