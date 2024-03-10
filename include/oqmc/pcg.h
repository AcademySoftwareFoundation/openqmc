// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/**
 * @file
 * @details An implementation of a pseudo random number generator (PRNG) by
 * Melissa E. O'Neill. Specifically the insecure PCG-RXS-M-RX-32 (commonly
 * referred to as PCG), as this meets the criteria required by higher level
 * functionality in the library. Further details are in 'PCG: A Family of
 * Simple Fast Space-Efficient Statistically Good Algorithms for Random Number
 * Generation'. This is also used to construct a hash function as described in
 * 'Hash Functions for GPU Rendering' by Mark Jarzynski and Marc Olano. The
 * constant coefficients for the PCG-RXS-M-RX-32 implementation were taken from
 * 'https:// github.com/imneme/pcg-c'.
 */

#pragma once

#include "gpu.h"

#include <cassert>
#include <cstdint>

namespace oqmc
{

namespace pcg
{

/**
 * @brief State transition function.
 * @details Transition state using an LCG to increment the PRNG index along the
 * sequence. Incrementing the input state can be used to select a new sequence
 * stream. Note that you may want to use the higher level functionality below.
 *
 * @param [in] state Internal state of the PRNG.
 * @return State after incrementation of index.
 *
 * @pre State must have been initialised using an init function.
 */
OQMC_HOST_DEVICE constexpr std::uint32_t stateTransition(std::uint32_t state)
{
	// LCG
	return state * 747796405u + 2891336453u;
}

/**
 * @brief Output permutation function.
 * @details Output permutation of the PRNG state, resulting in a usable
 * random value with good statistical properties. This implements the three
 * permutation operations used in PCG-RXS-M-32. Note you may want to use the
 * higher level functionality below.
 *
 * @param [in] state Internal state of the PRNG.
 * @return Output PRNG number from sequence.
 *
 * @pre State must have been initialised using an init function.
 */
OQMC_HOST_DEVICE constexpr std::uint32_t output(std::uint32_t state)
{
	// RXS
	state ^= state >> (4 + (state >> 28));

	// M
	state *= 277803737u;

	// XS
	state ^= state >> 22;

	return state;
}

/**
 * @brief Default initialise the PRNG state.
 * @details If given no seed, initialise the PRNG state using a zero value.
 * This must be done prior to passing the state to other functionality within
 * this file.
 *
 * @return Initialised internal state of the PRNG.
 */
OQMC_HOST_DEVICE constexpr std::uint32_t init()
{
	return stateTransition(0);
}

/**
 * @brief Initialise the PRNG state using a seed value.
 * @details If given a seed, initialise the PRNG state using said seed value.
 * This must be done prior to passing the state to other functionality within
 * this file.
 *
 * @param [in] seed Seed value for PRNG sequence.
 * @return Initialised internal state of the PRNG.
 */
OQMC_HOST_DEVICE constexpr std::uint32_t init(std::uint32_t seed)
{
	return init() + seed;
}

/**
 * @brief Compute a hash value based on an input key.
 * @details Given an input key, compute a random hash value. This can be used
 * to initialise a system or to compute an array of random values in parallel.
 *
 * @param [in] key Hash key value.
 * @return Output hash value.
 */
OQMC_HOST_DEVICE constexpr std::uint32_t hash(std::uint32_t key)
{
	key = stateTransition(key);
	return output(key);
}

/**
 * @brief Compute a random number from the PRNG sequence.
 * @details Given a PRNG state, compute the next random number, and iterate the
 * state for the next value in the sequence. Useful for generating high quality
 * random numbers when computing them sequentially. The output number will be
 * in the range of an unsigned 32 bit integer. For floating point numbers, this
 * integer value needs to be converted into a floating point representation by
 * the calling code.
 *
 * @param [in, out] state State value of the PRNG. Will be mutated.
 * @return Output PRNG value from the sequence.
 *
 * @pre State must have been initialised using an init function.
 */
OQMC_HOST_DEVICE constexpr std::uint32_t rng(std::uint32_t& state)
{
	state = stateTransition(state);
	return output(state);
}

/**
 * @brief Compute an unsigned random integer within 0-bounded half-open range.
 * @details Given a range defined using a single unsigned integer, use a PRNG
 * to compute a random integer value within that range. This function will not
 * introduce any statistical bias that is typically present when using more
 * naive methods, such as a modulo operator.
 *
 * From the following paper: https://doi.org/10.48550/arXiv.1805.10941. Note
 * that this algorithm (see Java) reduces the number of expected divisions to
 * almost one. However, low-order bits which can be weak in some PRNGs pass
 * through to the output. PCG has strong low-order bits. For low-discrepency
 * sequences it is recomended to use a multiplcation method instead. This will
 * preserve good properties in the correlation between outputs of the sequence.
 *
 * @param [in] range Exclusive end of integer range. Greater than zero.
 * @param [in, out] state State value of the PRNG. Will be mutated.
 * @return Output PRNG value within integer range.
 *
 * @pre State must have been initialised using an init function.
 */
OQMC_HOST_DEVICE constexpr std::uint32_t rngBounded(std::uint32_t range,
                                                    std::uint32_t& state)
{
	assert(range > 0);

	auto x = rng(state);
	auto r = x % range;

	while(x - r > (-range))
	{
		x = rng(state);
		r = x % range;
	}

	return r;
}

/**
 * @brief Compute an unsigned random integer within half-open range.
 * @details Given a range defined using two unsigned integers, use a PRNG to
 * compute a random integer value within that range. This function will not
 * introduce any statistical bias that is typically present when using more
 * naive methods, such as a modulo operator.
 *
 * @param [in] begin Inclusive beginning of integer range. Less than end.
 * @param [in] end Exclusive end of integer range. Greater than begin.
 * @param [in, out] state State value of the PRNG. Will be mutated.
 * @return Output PRNG value within integer range.
 *
 * @pre State must have been initialised using an init function.
 */
OQMC_HOST_DEVICE constexpr std::uint32_t
rngBounded(std::uint32_t begin, std::uint32_t end, std::uint32_t& state)
{
	assert(begin < end);

	const auto range = end - begin;
	return rngBounded(range, state) + begin;
}

} // namespace pcg

} // namespace oqmc
