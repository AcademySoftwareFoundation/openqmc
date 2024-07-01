// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#pragma once

#include <oqmc/gpu.h>
#include <oqmc/sampler.h>
#include <oqmc/state.h>
#include <oqmc/unused.h>

#include <cstddef>
#include <cstdint>

class RngImpl
{
	friend oqmc::SamplerInterface<RngImpl>;

	static constexpr std::size_t cacheSize = 0;
	static void initialiseCache(void* cache);

	OQMC_HOST_DEVICE RngImpl() = default;
	OQMC_HOST_DEVICE RngImpl(oqmc::State64Bit state);
	OQMC_HOST_DEVICE RngImpl(int x, int y, int frame, int index,
	                         const void* cache);

	OQMC_HOST_DEVICE RngImpl newDomain(int key) const;
	OQMC_HOST_DEVICE RngImpl newDomainDistrib(int key, int index) const;
	OQMC_HOST_DEVICE RngImpl newDomainSplit(int key, int size, int index) const;

	template <int Size>
	OQMC_HOST_DEVICE void drawSample(std::uint32_t sample[Size]) const;

	template <int Size>
	OQMC_HOST_DEVICE void drawRnd(std::uint32_t rnd[Size]) const;

	oqmc::State64Bit state;
};

inline void RngImpl::initialiseCache(void* cache)
{
	OQMC_MAYBE_UNUSED(cache);
}

inline RngImpl::RngImpl(oqmc::State64Bit state) : state(state)
{
}

inline RngImpl::RngImpl(int x, int y, int frame, int index, const void* cache)
    : state(x, y, frame, index)
{
	OQMC_MAYBE_UNUSED(cache);
	state = state.pixelDecorrelate();
}

inline RngImpl RngImpl::newDomain(int key) const
{
	return {state.newDomain(key)};
}

inline RngImpl RngImpl::newDomainDistrib(int key, int index) const
{
	return {state.newDomainDistrib(key, index)};
}

inline RngImpl RngImpl::newDomainSplit(int key, int size, int index) const
{
	return {state.newDomainSplit(key, size, index)};
}

template <int Size>
void RngImpl::drawSample(std::uint32_t sample[Size]) const
{
	drawRnd<Size>(sample);
}

template <int Size>
void RngImpl::drawRnd(std::uint32_t rnd[Size]) const
{
	state.drawRnd<Size>(rnd);
}

using RngSampler = oqmc::SamplerInterface<RngImpl>;
