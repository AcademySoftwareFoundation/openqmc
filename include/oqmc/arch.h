// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/**
 * @file
 * @details Defines what architecture to target. This is based on what features
 * are enabled by the compiler using the build options.
 */

#pragma once

#if defined(OQMC_FORCE_SCALAR)

#define OQMC_ARCH_SCALAR

#elif defined(__AVX2__)

#define OQMC_ARCH_AVX

#elif defined(__SSE2__)

#define OQMC_ARCH_SSE

#elif defined(__ARM_NEON)

#define OQMC_ARCH_ARM

#else

#define OQMC_ARCH_SCALAR

#endif
