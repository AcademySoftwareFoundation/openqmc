// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/// @file
/// @details Functionality around floating point operations, such as conversion
/// from integer to floating point representation.

#pragma once

#include "gpu.h"

#include <cstdint>

/// @defgroup utilities Utilities API
/// Simple utilities API and related functions.
///
/// This module captures an assortment of utility functions used in OpenQMC
/// to build higher level components. However, they can also be used directly
/// if needed and might be useful for other purposes, or when constructing new
/// sampler types.
///
/// Utilities cover the following functions:
///
/// - Integer to floating point conversion.
/// - Encoding and decoding integer values.
/// - Reversing and rotating integer bits.
/// - Randomising and shuffling integers.
/// - Mapping integers to a finite range.
/// - Randomised table access.

namespace oqmc
{

constexpr auto floatOneOverTwoPower32 = 1.0f / (1ull << 32); ///< 0x1p-32

/// Convert an integer into a [0, 1) float.
///
/// Given any representable 32 bit unsigned integer, scale the value into a [0,
/// 1) floating point representation. Note that this operation is lossy and may
/// not be reversible.
///
/// @ingroup utilities
/// @param [in] value Input integer value within the range [0, 2^32).
/// @return Floating point number within the range [0, 1).
OQMC_HOST_DEVICE inline float uintToFloat(std::uint32_t value)
{
	// There are various methods for converting an integer to a float, each with
	// a different balance of speed, quality and complexity.

	// Option 1: Default Conversion and Clamp Maximum
	//
	// A simple method is to cast the integer to a floating point
	// type, using the default runtime rounding mode (round nearest), and then
	// multiply by the reciprocal of the (exclusive) integer maximum (1/2^32).
	// This method is fast and retains a lot of precision, but has the
	// undesirable property of floating point values rounding up to the nearest
	// representable number to reduce error. This gives the potential for an
	// output equal to one, requiring a min operation. It also means the
	// probability density of representable values is not uniform. For example,
	// output 0.5 is 50% more likely than the next value.

	// Option 2: Truncate Precision and Default Conversion
	//
	// Marc Reynolds gives an option where 8 bits of precision is first removed
	// prior to division:
	// (http://marc-b-reynolds.github.io/distribution/2017/01/17/DenseFloat.html)
	// This removes the need for any min operations, but also loses precision
	// below one half; significant precision is lost for small values.

	// Option 3: Count Leading Zeros and Bitwise Manipulation
	//
	// A high-quality reference method is detailed in Section 2.1 of
	// 'Quasi-Monte Carlo Algorithms (not only) for Graphics Software' by
	// Keller, Wächter and Binder. This remaps the integer bits to the floating
	// point exponent and mantissa to provide optimal uniform probability
	// density. The computational cost however is high, due to instructions
	// for counting leading zeroes and manipulating the mantissa and exponent
	// bit patterns directly. It also uses a bitwise cast that can impact
	// optimiser analysis on some platforms. See a97ad21 for details.

	// Option 4: Adjust Integer and Default Conversion (Current Implementation)
	//
	// The following implementation provides results identical to the option 3,
	// while using a simpler approach that is more efficient on common
	// architectures.
	//
	// The reference method generates values equivalent to modifying the runtime
	// environment to round down, and then performing the conversion and scale:
	//		`std::fesetround(FE_DOWNWARD);`
	// 		`return static_cast<float>(value) * floatOneOverTwoPower32;`
	//
	// Unfortunately, modifying the runtime rounding mode is often expensive,
	// prohibited or ignored (see #pragma STDC FENV_ACCESS). It also affects
	// subsequent operations unless the previous state is restored.

	// However, it is possible to achieve the same effect by adjusting the
	// integer so that its conversion to floating point will always round down
	// with the default runtime mode (round nearest). This retains the optimal
	// uniform probability density at a lower computational cost.

	// When an integer is converted to floating point, the hardware identifies
	// the position of the leading one bit. The mantissa stores the following 23
	// bits, while the exponent encodes the position of that leading one.
	// The next three bits (guard, round, and sticky) govern the rounding
	// decision. The sticky bit represents the logical OR of all bits shifted
	// out beyond the round bit.

	// For example, consider a 32-bit input integer with three leading zeroes:
	//
	//        <-----------32 bits------------>
	// Input: 0001mmmmmmmmmmmmmmmmmmmmmmmgrsss
	//           |<-------23 bits------->|
	//			 1                       g
	// Key:
	// 		0 = leading zeros
	//		1 = leading one
	//		m = mantissa
	//		g = guard
	// 		r = round
	//		s = sticky
	//
	// To force rounding down during float conversion, it is both necessary and
	// sufficient to clear the guard bit.
	//
	// Fortunately, the guard bit (when present) is always located 24 bits
	// to the right of the leading one. This allows it to be cleared
	// efficiently without explicitly determining the position of either bit.
	// The integer is shifted right by 24 bits to generate a mask that clears
	// the guard bit and leaves the mantissa unmodified.

	// For example, with three leading zeroes the following bit patterns
	// are used:
	//
	// Input : 0001mmmm mmmmmmmm mmmmmmmm mmmgrsss
	// Mask  : 00000000 00000000 00000000 0001mmmm  	mask = input >> 24
	// Safe  : 0001mmmm mmmmmmmm mmmmmmmm mmm0????  	safe = input & ~mask
	//                                       ^
	//                              guard bit cleared
	//
	// The round and sticky bits are left undefined but do not affect the
	// outcome. With the guard bit cleared, the adjusted integer lies
	// strictly below the rounding tie-break, ensuring that both the round
	// and sticky bits are ignored.
	//
	// For inputs less than 2^24, the conversion to floating point is exact.
	// In this case, the mask evaluates to zero and the operation has no effect.

	// On common architectures this reduces to two or three instructions.
	// For example, ARMv8 requires two instructions: one that combines the
	// shift-not-and operations, and one for the conversion to float with
	// free scale by power-of-two constant.

	const auto mask = value >> 24;
	const auto safe = value & ~mask;

	return static_cast<float>(safe) * floatOneOverTwoPower32;
}

} // namespace oqmc
