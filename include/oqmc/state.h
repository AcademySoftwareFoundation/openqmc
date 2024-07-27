// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/// @file
/// @details Sampler state implementation.

#pragma once

#include "encode.h"
#include "gpu.h"
#include "pcg.h"

#include <cassert>
#include <cstdint>

namespace oqmc
{

/// Generic sampler state type.
/// This type is used to represent the state of higher level sampler
/// implementations. The size of the type is carefully handled to make sure it
/// is appropriate to pass-by-value. This allows for efficient functional style
/// use of the higher level API, an important requirement of the API design.
/// This type also provides functionality to mutate the state when building new
/// domains, along with the computation of generic PRNG values.
struct State64Bit
{
	static constexpr auto maxIndexBitSize = 16;   ///< 2^16 index upper limit.
	static constexpr auto maxIndexSize = 1 << 16; ///< 2^16 index upper limit.
	static constexpr auto spatialEncodeBitSizeX = 6; ///< 64 pixels in x.
	static constexpr auto spatialEncodeBitSizeY = 6; ///< 64 pixels in y.
	static constexpr auto temporalEncodeBitSize = 4; ///< 16 pixels in time.

	static_assert(spatialEncodeBitSizeX == spatialEncodeBitSizeY,
	              "Encoding must have equal resolution in x and y");

	/// Construct an invalid object.
	/// Create a placeholder object to allocate containers, etc. The resulting
	/// object is invalid, and you should initialise it by replacing the object
	/// with another from a parametrised constructor.
	/*AUTO_DEFINED*/ State64Bit() = default;

	/// Parametrised pixel constructor.
	/// Create an object based on the pixel, frame and sample indices. Once
	/// constructed the state object is valid and ready to use. Pixels are
	/// correlated by default, use pixelDecorrelate() to decorrelate pixels.
	///
	/// @param [in] x Pixel coordinate on the x axis.
	/// @param [in] y Pixel coordinate on the y axis.
	/// @param [in] frame Time index value.
	/// @param [in] index Sample index. Must be positive.
	OQMC_HOST_DEVICE State64Bit(int x, int y, int frame, int index);

	/// Decorrelate state between pixels.
	/// Using the pixelId, randomise the object state so that correlation
	/// between pixels is removed. You may want to call this after initial
	/// construction of which leaves pixels correlated as default.
	///
	/// @return Decorrelated state object.
	OQMC_HOST_DEVICE State64Bit pixelDecorrelate() const;

	/// @copydoc oqmc::SamplerInterface::newDomain()
	OQMC_HOST_DEVICE State64Bit newDomain(int key) const;

	/// @copydoc oqmc::SamplerInterface::newDomainSplit()
	OQMC_HOST_DEVICE State64Bit newDomainSplit(int key, int size,
	                                           int index) const;

	/// @copydoc oqmc::SamplerInterface::newDomainDistrib()
	OQMC_HOST_DEVICE State64Bit newDomainDistrib(int key, int index) const;

	/// @copydoc oqmc::SamplerInterface::drawRnd()
	template <int Size>
	OQMC_HOST_DEVICE void drawRnd(std::uint32_t rnd[Size]) const;

	std::uint32_t patternId; ///< Identifier for domain pattern.
	std::uint16_t sampleId;  ///< Identifier for sample index.
	std::uint16_t pixelId;   ///< Identifier for pixel position.
};

/// Compute 16-bit key from index.
/// Given a sample index, compute a key value based on the top 16-bits of the
/// integer range. Use computeIndexId() to compute the corrosponding new index
/// to pair with the key.
///
/// @param [in] index Sample index.
/// @return 16-bit Key value.
OQMC_HOST_DEVICE constexpr int computeIndexKey(int index)
{
	constexpr auto offset = State64Bit::maxIndexBitSize;
	return index >> offset;
}

/// Compute new 16-bit index from index.
/// Given a sample index, compute a new index value based on the bottom 16-bits
/// of the integer range. Use computeIndexKey() to compute the corrosponding key
/// value to pair with the new index.
///
/// @param [in] index Sample index.
/// @return New 16-bit index.
OQMC_HOST_DEVICE constexpr int computeIndexId(int index)
{
	constexpr auto mask = State64Bit::maxIndexSize - 1;
	return index & mask;
}

inline State64Bit::State64Bit(int x, int y, int frame, int index)
{
	assert(index >= 0);

	const auto indexKey = computeIndexKey(index);
	const auto indexId = computeIndexId(index);

	constexpr auto xBits = State64Bit::spatialEncodeBitSizeX;
	constexpr auto yBits = State64Bit::spatialEncodeBitSizeY;
	constexpr auto zBits = State64Bit::temporalEncodeBitSize;

	const auto pixelId = encodeBits16<xBits, yBits, zBits>({x, y, frame});

	this->patternId = pcg::init(indexKey);
	this->sampleId = indexId;
	this->pixelId = pixelId;
}

inline State64Bit State64Bit::pixelDecorrelate() const
{
	return newDomain(pixelId);
}

inline State64Bit State64Bit::newDomain(int key) const
{
	auto ret = *this;
	ret.patternId = pcg::stateTransition(patternId + key);

	return ret;
}

inline State64Bit State64Bit::newDomainSplit(int key, int size, int index) const
{
	assert(size > 0);
	assert(index >= 0);

	const auto indexKey = computeIndexKey(sampleId * size + index);
	const auto indexId = computeIndexId(sampleId * size + index);

	auto ret = newDomain(key).newDomain(indexKey);
	ret.sampleId = indexId;

	return ret;
}

inline State64Bit State64Bit::newDomainDistrib(int key, int index) const
{
	assert(index >= 0);

	const auto indexKey = computeIndexKey(index);
	const auto indexId = computeIndexId(index);

	auto ret = newDomain(key).newDomain(indexKey).newDomain(sampleId);
	ret.sampleId = indexId;

	return ret;
}

template <int Size>
void State64Bit::drawRnd(std::uint32_t rnd[Size]) const
{
	static_assert(Size >= 0, "Draw size greater or equal to zero.");

	auto rngState = patternId + sampleId;

	for(int i = 0; i < Size; ++i)
	{
		rnd[i] = pcg::rng(rngState);
	}
}

static_assert(sizeof(State64Bit) == 8, "State64Bit must be 8 bytes in size.");

} // namespace oqmc
