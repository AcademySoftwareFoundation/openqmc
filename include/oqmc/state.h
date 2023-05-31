// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/**
 * @file
 * @details Sampler state implementation.
 */

#pragma once

#include "encode.h"
#include "gpu.h"
#include "pcg.h"

#include <cassert>
#include <cstdint>

namespace oqmc
{

/**
 * @brief Generic sampler state type.
 * @details This type is used to represent the state of higher level sampler
 * implementations. The size of the type is carefully handled to make sure it is
 * appropriate to pass-by-value. This allows for efficient functional style use
 * of the higher level API, an important requirement of the API design. This
 * type also provides functionality to mutate the state when building new
 * domains, along with the computation of generic PRNG values.
 */
struct State64Bit
{
	static constexpr auto maxIndexSize = 0x10000; ///< 2^16 index upper limit.
	static constexpr auto spatialEncodeBitSizeX = 6; ///< 64 pixels in x.
	static constexpr auto spatialEncodeBitSizeY = 6; ///< 64 pixels in y.
	static constexpr auto temporalEncodeBitSize = 4; ///< 16 pixels in time.

	static_assert(spatialEncodeBitSizeX == spatialEncodeBitSizeY,
	              "Encoding must have equal resolution in x and y");

	/**
	 * @brief Construct an invalid object.
	 * @details Create a placeholder object to allocate containers, etc. The
	 * resulting object is invalid, and you should initialise it by replacing
	 * the object with another from a parametrised constructor.
	 */
	/*AUTO_DEFINED*/ State64Bit() = default;

	/**
	 * @brief Parametrised pixel constructor.
	 * @details Create an object based on the pixel, frame and sample indices.
	 * Once constructed the state object is valid and ready to use.
	 *
	 * Values for sampleId should be within the [0, 2^16) range; this constraint
	 * allows for possible optimisations in the implementations.
	 *
	 * @param x [in] Pixel coordinate on the x axis.
	 * @param y [in] Pixel coordinate on the y axis.
	 * @param frame [in] Time index value.
	 * @param sampleId [in] Sample index. Must be within [0, 2^16).
	 */
	OQMC_HOST_DEVICE State64Bit(int x, int y, int frame, int sampleId);

	/*
	 * @brief Decorrelate state between pixels.
	 * @details Using the pixelId, randomise the object state so that
	 * correlation between pixels is removed. You may want to call this after
	 * initial construction of the object.
	 */
	OQMC_HOST_DEVICE void pixelDecorrelate();

	/// @copydoc oqmc::SamplerInterface::newDomain()
	OQMC_HOST_DEVICE State64Bit newDomain(int key) const;

	/// @copydoc oqmc::SamplerInterface::newDomainDistrib()
	OQMC_HOST_DEVICE State64Bit newDomainDistrib(int key) const;

	/// @copydoc oqmc::SamplerInterface::newDomainSplit()
	OQMC_HOST_DEVICE State64Bit newDomainSplit(int key, int size) const;

	/// @copydoc oqmc::SamplerInterface::nextDomainIndex()
	OQMC_HOST_DEVICE State64Bit nextDomainIndex() const;

	/// @copydoc oqmc::SamplerInterface::drawRnd()
	template <int Size>
	OQMC_HOST_DEVICE void drawRnd(std::uint32_t rnds[Size]) const;

	/*
	 * @brief Find the sum of sampleId and an integer value.
	 * @details Given the current sampleId value of the object, return the sum
	 * of the sampleId and an integer argument. The result is asserted against
	 * integer overflow.
	 *
	 * @param [in] value Integer to add to sampleId.
	 * @return Result of summing sampleId and value.
	 */
	OQMC_HOST_DEVICE std::uint16_t sampleIdAdd(int value) const;

	/*
	 * @brief Find the product of sampleId and an integer value.
	 * @details Given the current sampleId value of the object, return the
	 * product of the sampleId and an integer argument. The result is asserted
	 * against integer overflow.
	 *
	 * @param [in] value Integer to multiply with sampleId.
	 * @return Result of multiplying sampleId and value.
	 */
	OQMC_HOST_DEVICE std::uint16_t sampleIdMult(int value) const;

	std::uint32_t patternId; ///< Identifier for domain pattern.
	std::uint16_t sampleId;  ///< Identifier for sample index.
	std::uint16_t pixelId;   ///< Identifier for pixel position.
};

inline State64Bit::State64Bit(int x, int y, int frame, int sampleId)
{
	assert(sampleId >= 0);
	assert(sampleId < maxIndexSize);

	constexpr auto xBits = State64Bit::spatialEncodeBitSizeX;
	constexpr auto yBits = State64Bit::spatialEncodeBitSizeY;
	constexpr auto zBits = State64Bit::temporalEncodeBitSize;

	const auto pixelId = encodeBits16<xBits, yBits, zBits>({x, y, frame});

	this->patternId = pcg::init();
	this->sampleId = sampleId;
	this->pixelId = pixelId;
}

inline void State64Bit::pixelDecorrelate()
{
	*this = newDomain(pixelId);
}

inline State64Bit State64Bit::newDomain(int key) const
{
	auto ret = *this;
	ret.patternId = pcg::stateTransition(patternId + key);

	return ret;
}

inline State64Bit State64Bit::newDomainDistrib(int key) const
{
	auto ret = newDomain(key).newDomain(sampleId);
	ret.sampleId = 0;

	return ret;
}

inline State64Bit State64Bit::newDomainSplit(int key, int size) const
{
	assert(size > 0);

	auto ret = newDomain(key);
	ret.sampleId = sampleIdMult(size);

	return ret;
}

inline State64Bit State64Bit::nextDomainIndex() const
{
	auto ret = *this;
	ret.sampleId = sampleIdAdd(1);

	return ret;
}

template <int Size>
void State64Bit::drawRnd(std::uint32_t rnds[Size]) const
{
	auto rngState = patternId + sampleId;

	for(int i = 0; i < Size; ++i)
	{
		rnds[i] = pcg::rng(rngState);
	}
}

inline std::uint16_t State64Bit::sampleIdAdd(int value) const
{
	const auto sampleId = static_cast<int>(this->sampleId) + value;

	assert(sampleId >= 0);
	assert(sampleId < maxIndexSize);

	return sampleId;
}

inline std::uint16_t State64Bit::sampleIdMult(int value) const
{
	const auto sampleId = static_cast<int>(this->sampleId) * value;

	assert(sampleId >= 0);
	assert(sampleId < maxIndexSize);

	return sampleId;
}

static_assert(sizeof(State64Bit) == 8, "State64Bit must be 8 bytes in size.");

} // namespace oqmc
