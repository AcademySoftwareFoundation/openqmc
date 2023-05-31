// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/**
 * @file
 * @details Lattice sampler implementation.
 */

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

/**
 * @brief Lattice sampler implemention without public interface.
 * @details Private implementation details of the lattice sampler. Use
 * the aliased LatticeSampler type below to access the sampler via the
 * SamplerInterface.
 *
 * The implementation uses the generator vector from Hickernell et al. in
 * 'Weighted compound integration rules with higher order convergence for
 * all N' to construct a 4D lattice. This is then made into a progressive
 * sequence using a scalar based on a radical inversion of the sample index.
 * Randomisation uses toroidal shifts.
 *
 * This sampler has no cache initialisation cost, it generates all samples on
 * the fly without touching memory. Runtime performance is also high with a
 * relatively low computation cost for a single draw sample call. However the
 * rate of integration per pixel can be lower when compared to other samplers.
 */
class LatticeImpl
{
	friend SamplerInterface<LatticeImpl>;

	static constexpr std::size_t cacheSize = 0;
	static void initialiseCache(void* cache);

	/*AUTO_DEFINED*/ LatticeImpl() = default;
	OQMC_HOST_DEVICE LatticeImpl(State64Bit state);
	OQMC_HOST_DEVICE LatticeImpl(int x, int y, int frame, int sampleId,
	                             const void* cache);

	OQMC_HOST_DEVICE LatticeImpl newDomain(int key) const;
	OQMC_HOST_DEVICE LatticeImpl newDomainDistrib(int key) const;
	OQMC_HOST_DEVICE LatticeImpl newDomainSplit(int key, int size) const;
	OQMC_HOST_DEVICE LatticeImpl nextDomainIndex() const;

	template <int Size>
	OQMC_HOST_DEVICE void drawSample(std::uint32_t samples[Size]) const;

	template <int Size>
	OQMC_HOST_DEVICE void drawRnd(std::uint32_t rnds[Size]) const;

	State64Bit state;
};

inline void LatticeImpl::initialiseCache(void* cache)
{
	OQMC_MAYBE_UNUSED(cache);
}

inline LatticeImpl::LatticeImpl(State64Bit state) : state(state)
{
}

inline LatticeImpl::LatticeImpl(int x, int y, int frame, int sampleId,
                                const void* cache)
    : state(x, y, frame, sampleId)
{
	OQMC_MAYBE_UNUSED(cache);
	state.pixelDecorrelate();
}

inline LatticeImpl LatticeImpl::newDomain(int key) const
{
	return {state.newDomain(key)};
}

inline LatticeImpl LatticeImpl::newDomainDistrib(int key) const
{
	return {state.newDomainDistrib(key)};
}

inline LatticeImpl LatticeImpl::newDomainSplit(int key, int size) const
{
	return {state.newDomainSplit(key, size)};
}

inline LatticeImpl LatticeImpl::nextDomainIndex() const
{
	return {state.nextDomainIndex()};
}

template <int Size>
void LatticeImpl::drawSample(std::uint32_t samples[Size]) const
{
	shuffledRotatedLattice<Size>(state.sampleId, state.patternId, samples);
}

template <int Size>
void LatticeImpl::drawRnd(std::uint32_t rnds[Size]) const
{
	state.drawRnd<Size>(rnds);
}

using LatticeSampler = SamplerInterface<LatticeImpl>;

} // namespace oqmc
