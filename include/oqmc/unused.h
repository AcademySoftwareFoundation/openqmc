// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/**
 * @file
 * @details Defines a macro to declare an expression as potentially unused.
 * This is needed for older versions of C++ without the maybe_unused attribute.
 */

#pragma once

#define OQMC_MAYBE_UNUSED(exp) (void)(exp)
