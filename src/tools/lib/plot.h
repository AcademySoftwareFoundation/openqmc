// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#pragma once

#include "abi.h"

// NOLINTNEXTLINE: C style naming
OQMC_CABI bool oqmc_plot_shape(const char* shape, int nsamples, int resolution,
                               float* out);

// NOLINTNEXTLINE: C style naming
OQMC_CABI bool oqmc_plot_zoneplate(const char* sampler, int nsamples,
                                   int resolution, float* out);

// NOLINTNEXTLINE: C style naming
OQMC_CABI bool oqmc_plot_error(const char* shape, const char* sampler,
                               int nsequences, int nsamples, float* out);

// NOLINTNEXTLINE: C style naming
OQMC_CABI bool oqmc_plot_error_filter_space(const char* shape,
                                            const char* sampler, int resolution,
                                            int nsamples, int nsigma,
                                            float sigmaMin, float sigmaStep,
                                            float* out);

// NOLINTNEXTLINE: C style naming
OQMC_CABI bool oqmc_plot_error_filter_time(const char* shape,
                                           const char* sampler, int resolution,
                                           int depth, int nsamples, int nsigma,
                                           float sigmaMin, float sigmaStep,
                                           float* out);
