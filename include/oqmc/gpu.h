// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/// @file
/// @details Defines function specifiers which allows the author to inform the
/// compiler a function should be callable on the GPU.

#pragma once

#if defined(__CUDACC__)
#define OQMC_HOST_DEVICE __host__ __device__
#else
#define OQMC_HOST_DEVICE
#endif
