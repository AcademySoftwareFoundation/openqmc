# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

import sys
import os
import ctypes
import numpy as np

module = np.ctypeslib.load_library("libtools", os.environ["TOOLSPATH"])

module.oqmc_benchmark.restype = ctypes.c_bool
module.oqmc_benchmark.argtypes = [
    ctypes.c_char_p,
    ctypes.c_char_p,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_int),
]


def benchmark(sampler, measurement, nsamples, ndims):
    time = ctypes.c_int(0)
    valid = module.oqmc_benchmark(
        sampler, measurement, nsamples, ndims, ctypes.byref(time)
    )

    if not valid:
        sys.exit()

    return time.value


module.oqmc_frequency_continuous.restype = ctypes.c_bool
module.oqmc_frequency_continuous.argtypes = [
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    np.ctypeslib.ndpointer(),
    np.ctypeslib.ndpointer(),
]


def frequency(nsequences, nsamples, ndims, depthA, depthB, resolution, src):
    spectrum = np.zeros((resolution, resolution), dtype=np.float32)
    valid = module.oqmc_frequency_continuous(
        nsequences, nsamples, ndims, depthA, depthB, resolution, src, spectrum
    )

    if not valid:
        sys.exit()

    return spectrum


module.oqmc_generate.restype = ctypes.c_bool
module.oqmc_generate.argtypes = [
    ctypes.c_char_p,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    np.ctypeslib.ndpointer(),
]


def generate(name, nsequences, nsamples, ndims):
    points = np.zeros((nsequences, nsamples, ndims), dtype=np.float32)
    valid = module.oqmc_generate(name, nsequences, nsamples, ndims, points)

    if not valid:
        sys.exit()

    return points


module.oqmc_optimise.restype = ctypes.c_bool
module.oqmc_optimise.argtypes = [
    ctypes.c_char_p,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    np.ctypeslib.ndpointer(),
    np.ctypeslib.ndpointer(),
    np.ctypeslib.ndpointer(),
    np.ctypeslib.ndpointer(),
]


def optimise(name, ntests, niterations, nsamples, resolution, depth, seed):
    module.oqmc_progress_off()

    keys = np.zeros((resolution, resolution, depth), dtype=np.uint32)
    ranks = np.zeros((resolution, resolution, depth), dtype=np.uint32)
    estimates = np.zeros((resolution, resolution, depth), dtype=np.float32)
    frequencies = np.zeros((resolution, resolution, depth), dtype=np.float32)
    valid = module.oqmc_optimise(
        name,
        ntests,
        niterations,
        nsamples,
        resolution,
        depth,
        seed,
        keys,
        ranks,
        estimates,
        frequencies,
    )

    module.oqmc_progress_on()

    if not valid:
        sys.exit()

    return [keys, ranks, estimates, frequencies]


module.oqmc_plot_shape.restype = ctypes.c_bool
module.oqmc_plot_shape.argtypes = [
    ctypes.c_char_p,
    ctypes.c_int,
    ctypes.c_int,
    np.ctypeslib.ndpointer(),
]


def plot_shape(shape, nsamples, resolution):
    image = np.zeros((resolution, resolution), dtype=np.float32)
    valid = module.oqmc_plot_shape(shape, nsamples, resolution, image)

    if not valid:
        sys.exit()

    return image


module.oqmc_plot_zoneplate.restype = ctypes.c_bool
module.oqmc_plot_zoneplate.argtypes = [
    ctypes.c_char_p,
    ctypes.c_int,
    ctypes.c_int,
    np.ctypeslib.ndpointer(),
]


def plot_zoneplate(sampler, nsamples, resolution):
    image = np.zeros((resolution, resolution), dtype=np.float32)
    valid = module.oqmc_plot_zoneplate(sampler, nsamples, resolution, image)

    if not valid:
        sys.exit()

    return image


module.oqmc_plot_error.restype = ctypes.c_bool
module.oqmc_plot_error.argtypes = [
    ctypes.c_char_p,
    ctypes.c_char_p,
    ctypes.c_int,
    ctypes.c_int,
    np.ctypeslib.ndpointer(),
]


def plot_error(shape, sampler, nsequences, nsamples):
    errors = np.zeros((nsamples, 2), dtype=np.float32)
    valid = module.oqmc_plot_error(shape, sampler, nsequences, nsamples, errors)

    if not valid:
        sys.exit()

    return errors


module.oqmc_plot_error_filter_space.restype = ctypes.c_bool
module.oqmc_plot_error_filter_space.argtypes = [
    ctypes.c_char_p,
    ctypes.c_char_p,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_float,
    ctypes.c_float,
    np.ctypeslib.ndpointer(),
]


def plot_error_filter_space(
    shape, sampler, resolution, nsamples, nsigma, sigmaMin, sigmaStep
):
    errors = np.zeros((nsigma, 2), dtype=np.float32)
    valid = module.oqmc_plot_error_filter_space(
        shape, sampler, resolution, nsamples, nsigma, sigmaMin, sigmaStep, errors
    )

    if not valid:
        sys.exit()

    return errors


module.oqmc_plot_error_filter_time.restype = ctypes.c_bool
module.oqmc_plot_error_filter_time.argtypes = [
    ctypes.c_char_p,
    ctypes.c_char_p,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_float,
    ctypes.c_float,
    np.ctypeslib.ndpointer(),
]


def plot_error_filter_time(
    shape, sampler, resolution, depth, nsamples, nsigma, sigmaMin, sigmaStep
):
    errors = np.zeros((nsigma, 2), dtype=np.float32)
    valid = module.oqmc_plot_error_filter_time(
        shape, sampler, resolution, depth, nsamples, nsigma, sigmaMin, sigmaStep, errors
    )

    if not valid:
        sys.exit()

    return errors


module.oqmc_trace.restype = ctypes.c_bool
module.oqmc_trace.argtypes = [
    ctypes.c_char_p,
    ctypes.c_char_p,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    np.ctypeslib.ndpointer(),
]


def trace(
    name,
    scene,
    width,
    height,
    frame,
    numPixelSamples,
    numLightSamples,
    maxDepth,
    maxOpacity,
):
    module.oqmc_progress_off()

    image = np.zeros((height, width, 3), dtype=np.float32)
    valid = module.oqmc_trace(
        name,
        scene,
        width,
        height,
        frame,
        numPixelSamples,
        numLightSamples,
        maxDepth,
        maxOpacity,
        image,
    )

    module.oqmc_progress_on()

    if not valid:
        sys.exit()

    return image
