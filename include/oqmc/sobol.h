// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/// @file
/// @details Sobol sampler implementation.

#pragma once

#include "gpu.h"
#include "owen.h"
#include "sampler.h"
#include "state.h"

#include <cstddef>
#include <cstdint>

namespace oqmc
{

/// @cond
class SobolImpl
{
	// See SamplerInterface for public API documentation.
	friend SamplerInterface<SobolImpl>;

	static constexpr std::size_t cacheSize = 0;
	static void initialiseCache(void* cache);

	/*AUTO_DEFINED*/ SobolImpl() = default;
	OQMC_HOST_DEVICE SobolImpl(State64Bit state);
	OQMC_HOST_DEVICE SobolImpl(int x, int y, int frame, int index,
	                           const void* cache);

	OQMC_HOST_DEVICE SobolImpl newDomain(int key) const;
	OQMC_HOST_DEVICE SobolImpl newDomainSplit(int key, int size,
	                                          int index) const;
	OQMC_HOST_DEVICE SobolImpl newDomainDistrib(int key, int index) const;

	template <int Size>
	OQMC_HOST_DEVICE void drawSample(std::uint32_t sample[Size]) const;

	template <int Size>
	OQMC_HOST_DEVICE void drawRnd(std::uint32_t rnd[Size]) const;

	State64Bit state;
};

inline void SobolImpl::initialiseCache([[maybe_unused]] void* cache)
{
}

inline SobolImpl::SobolImpl(State64Bit state) : state(state)
{
}

inline SobolImpl::SobolImpl(int x, int y, int frame, int index,
                            [[maybe_unused]] const void* cache)
    : state(x, y, frame, index)
{
	state = state.pixelDecorrelate();
}

inline SobolImpl SobolImpl::newDomain(int key) const
{
	return {state.newDomain(key)};
}

inline SobolImpl SobolImpl::newDomainSplit(int key, int size, int index) const
{
	return {state.newDomainSplit(key, size, index)};
}

inline SobolImpl SobolImpl::newDomainDistrib(int key, int index) const
{
	return {state.newDomainDistrib(key, index)};
}

template <int Size>
void SobolImpl::drawSample(std::uint32_t sample[Size]) const
{
	shuffledScrambledSobol<Size>(state.sampleId, pcg::output(state.patternId),
	                             sample);
}

template <int Size>
void SobolImpl::drawRnd(std::uint32_t rnd[Size]) const
{
	state.drawRnd<Size>(rnd);
}
/// @endcond

/// Owen scrambled sobol sampler.
///
/// The implementation uses an elegant construction by Burley in 'Practical
/// Hash-based Owen Scrambling' for an Owen scrambled sequence, with the SZ
/// generator matrices by Ahmed et al. in 'SZ Sequences: Binary-Constructed
/// (0, 2^q)-Sequences' (SIGGRAPH Asia 2025) in place of the classic Sobol
/// direction numbers. Dimensions 0 and 1 are the same as Sobol, while every
/// pair of the four dimensions forms a (0, 2)-sequence in base 4, and the
/// four together a (0, 4)-sequence, so all 2D projections are well
/// stratified. This also includes performance improvements such as limiting
/// the index to 16 bits, pre-inverting the input and output matrices, and
/// reversing the index once per draw. Each matrix is factored into a closed
/// form program using the construction method by Abdalla G. M. Ahmed in 'An
/// Implementation Algorithm of 2D Sobol Sequence Fast, Elegant, and Compact'
/// (EGSR 2024).
///
/// This sampler has no cache initialisation cost, it generates all samples on
/// the fly without touching memory. However the cost per draw sample call is
/// computationally higher than other samplers. The quality of Owen scramble
/// sequences often outweigh this cost due to their random error cancellation
/// and incredibly high rate of integration for smooth functions.
///
/// @ingroup samplers
using SobolSampler = SamplerInterface<SobolImpl>;

} // namespace oqmc
