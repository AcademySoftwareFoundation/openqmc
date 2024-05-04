// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/**
 * @file
 * @details Functionality for remapping integer values to within a bounded range
 * while retaining quality properties of random numbers at low cost.
 */

#pragma once

#include "gpu.h"

#include <cassert>
#include <cstdint>

namespace oqmc
{

/**
 * @brief Compute an unsigned integer within 0-bounded half-open range.
 * @details Given a range defined using a single unsigned integer, map a full
 * range 32 bit unsigned integer into range.
 *
 * This function will avoid any division using a multiplication method that
 * preserves the high order bits. This means that PRNGs with weak low order
 * bits, as welll as QMC sequences, will both retain their good properties.
 *
 * Due to these reasons, this function should be prefered over other naive
 * methods such as a modulo operator. It does however still introduce a form of
 * bias at large non power-of-two ranges. This could be resolved with a simple
 * extension ivolving a rejection method. However such methods do not integrate
 * well with the larger design, and so are not used as a practical compromise.
 *
 * For further information see Section 4 in https://arxiv.org/abs/1805.10941 as
 * well as https://www.pcg-random.org/posts/bounded-rands.html from PCG.
 *
 * @param [in] value Full ranged 32 bit unsigned integer.
 * @param [in] range Exclusive end of integer range. Greater than zero.
 * @return Output value remapped within integer range.
 */
OQMC_HOST_DEVICE constexpr std::uint32_t uintToRange(std::uint32_t value,
                                                     std::uint32_t range)
{
	assert(range > 0);

	// Modern compilers will optimise this down to a standard mult instruction,
	// avoiding 64 bits, taking only the upper order bits in the EAX register.
	const auto x = static_cast<std::uint64_t>(value);
	const auto y = static_cast<std::uint64_t>(range);
	return x * y >> 32;
}

/**
 * @brief Compute an unsigned integer within half-open range.
 * @details Given a range defined using two unsigned integers, map a full range
 * 32 bit unsigned integer into range. This function is based upon the other
 * uint to range function and has the same properties.
 *
 * @param [in] value Full ranged 32 bit unsigned integer.
 * @param [in] begin Inclusive beginning of integer range. Less than end.
 * @param [in] end Exclusive end of integer range. Greater than begin.
 * @return Output value remapped within integer range.
 */
OQMC_HOST_DEVICE constexpr std::uint32_t
uintToRange(std::uint32_t value, std::uint32_t begin, std::uint32_t end)
{
	assert(begin < end);

	return uintToRange(value, end - begin) + begin;
}

} // namespace oqmc
