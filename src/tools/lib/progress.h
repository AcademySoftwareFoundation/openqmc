// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#pragma once

#include "abi.h"

#include <time.h> // NOLINT: can be a C header

// NOLINTNEXTLINE: C style naming
OQMC_CABI void oqmc_progress_on();

// NOLINTNEXTLINE: C style naming
OQMC_CABI void oqmc_progress_off();

// NOLINTNEXTLINE: C style naming
OQMC_CABI time_t oqmc_progress_start(const char* string, long int size);

// NOLINTNEXTLINE: C style naming
OQMC_CABI void oqmc_progress_end();

// NOLINTNEXTLINE: C style naming
OQMC_CABI void oqmc_progress_add(const char* string, long int size,
                                 long int index, time_t start);
