// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/// @file
/// @details An implementation of a pseudo random number generator (PRNG) by
/// Melissa E. O'Neill. Specifically the insecure PCG-RXS-M-RX-32 (commonly
/// referred to as PCG), as this meets the criteria required by higher level
/// functionality in the library. Further details are in 'PCG: A Family of
/// Simple Fast Space-Efficient Statistically Good Algorithms for Random Number
/// Generation'. This is also used to construct a hash function as described in
/// 'Hash Functions for GPU Rendering' by Mark Jarzynski and Marc Olano. The
/// constant coefficients for the PCG-RXS-M-RX-32 implementation were taken from
/// https://github.com/imneme/pcg-c.

#pragma once

#include "gpu.h"

#include <cassert>
#include <cstdint>

/// @defgroup rngs Rng API
/// Simple rng API and related functions.
///
/// This module outlines the RNG API which is used extensively as a building
/// block in sampler implementations. PCG is used as both a classic sequential
/// PRNG as-well-as a parallel hash function.
///
/// Here is a typical RNG example:
///
/// @code
/// auto state = oqmc::pcg::init(seed);
///
/// const auto rand0 = oqmc::pcg::rng(state);
/// const auto rand1 = oqmc::pcg::rng(state);
/// const auto rand2 = oqmc::pcg::rng(state);
/// const auto rand3 = oqmc::pcg::rng(state);
/// @endcode
///
/// Here is a typical hash example:
///
/// @code
/// const auto rand0 = oqmc::pcg::hash(0);
/// const auto rand1 = oqmc::pcg::hash(1);
/// const auto rand2 = oqmc::pcg::hash(2);
/// const auto rand3 = oqmc::pcg::hash(3);
/// @endcode
///
/// Notice how one is statefull while the other is stateless. For best results
/// use the RNG when possible. In both of these examples the rand values are
/// integers. Use the oqmc::uintToFloat() function to convert the integers to
/// floats.
///
/// When hashing, if you need to convert N inputs to 1 output, this can be
/// done by nesting the hash function. However, nesting just the result from
/// oqmc::pcg::stateTransition() is more optimal. If you do this make sure to
/// call oqmc::pcg::output() prior to using the hash value.
///
/// This for example can be done to create the hash values for nodes and leaves
/// in a tree. Which is especially fast if the intermediate values are re-used
/// for subtrees. This is how domains are computed in OpenQMC.

namespace oqmc
{

namespace pcg
{

/// State transition function.
///
/// Transition state using an LCG to increment the PRNG index along the
/// sequence. Incrementing the input state can be used to select a new sequence
/// stream. Note that you may want to use the higher level functionality below.
///
/// @ingroup rngs
/// @param [in] state Internal state of the PRNG.
/// @return State after incrementation of index.
/// @pre State must have been initialised using an init function.
OQMC_HOST_DEVICE constexpr std::uint32_t stateTransition(std::uint32_t state)
{
	// LCG
	return state * 747796405u + 2891336453u;
}

/// Output permutation function.
///
/// Output permutation of the PRNG state, resulting in a usable random value
/// with good statistical properties. This implements the three permutation
/// operations used in PCG-RXS-M-32. Note you may want to use the higher level
/// functionality below.
///
/// @ingroup rngs
/// @param [in] state Internal state of the PRNG.
/// @return Output PRNG number from sequence.
/// @pre State must have been initialised using an init function.
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

/// Default initialise the PRNG state.
///
/// If given no seed, initialise the PRNG state using a zero value. This must be
/// done prior to passing the state to other functionality within this file.
///
/// @ingroup rngs
/// @return Initialised internal state of the PRNG.
OQMC_HOST_DEVICE constexpr std::uint32_t init()
{
	return stateTransition(0);
}

/// Initialise the PRNG state using a seed value.
///
/// If given a seed, initialise the PRNG state using said seed value. This must
/// be done prior to passing the state to other functionality within this file.
///
/// @ingroup rngs
/// @param [in] seed Seed value for PRNG sequence.
/// @return Initialised internal state of the PRNG.
OQMC_HOST_DEVICE constexpr std::uint32_t init(std::uint32_t seed)
{
	return init() + seed;
}

/// Compute a hash value based on an input key.
///
/// Given an input key, compute a random hash value. This can be used to
/// initialise a system or to compute an array of random values in parallel.
///
/// @ingroup rngs
/// @param [in] key Hash key value.
/// @return Output hash value.
OQMC_HOST_DEVICE constexpr std::uint32_t hash(std::uint32_t key)
{
	key = stateTransition(key);
	return output(key);
}

/// Compute a random number from the PRNG sequence.
///
/// Given a PRNG state, compute the next random number, and iterate the state
/// for the next value in the sequence. Useful for generating high quality
/// random numbers when computing them sequentially. The output number will be
/// in the range of an unsigned 32 bit integer. For floating point numbers, this
/// integer value needs to be converted into a floating point representation by
/// the calling code.
///
/// @ingroup rngs
/// @param [in, out] state State value of the PRNG. Will be mutated.
/// @return Output PRNG value from the sequence.
/// @pre State must have been initialised using an init function.
OQMC_HOST_DEVICE constexpr std::uint32_t rng(std::uint32_t& state)
{
	state = stateTransition(state);
	return output(state);
}

} // namespace pcg

} // namespace oqmc
