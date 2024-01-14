# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

import os
import matplotlib.pyplot as plt
import numpy as np
import wrapper as oqmc

sequences_base = ("pmj", "sobol", "lattice")
sequences_complete = ("pmj", "pmjbn", "sobol", "sobolbn", "lattice", "latticebn")

shapes = {
    "qdisk": "quarter disk",
    "qgauss": "quarter gaussian",
    "bilin": "bilinear",
    "heavi": "orientated heaviside",
}


def save_light_dark_figure(id, function, *args, **kwargs):
    options = {"light": "default", "dark": "dark_background"}

    variable_name = "SAVEPLOT"
    if variable_name not in os.environ:
        function(*args, **kwargs)
        return

    variable_value = os.environ[variable_name]
    if variable_value not in options:
        function(*args, **kwargs)
        return

    def use_style(style):
        plt.style.use(style)

    def build_path(name, type):
        return "../../images/plots/{}-{}.png".format(name, type)

    def save_figure(path):
        plt.savefig(path, dpi=200, bbox_inches="tight", transparent=True)

    use_style(options[variable_value])

    function(*args, **kwargs)

    save_figure(build_path(id, variable_value))


def disable_tick(tick):
    tick.tick1line.set_visible(False)
    tick.tick2line.set_visible(False)
    tick.label1.set_visible(False)
    tick.label2.set_visible(False)


def eotf_inverse_sRGB(image):
    return np.where(
        image <= 0.0031308, image * 12.92, ((image ** (1 / 2.4)) * 1.055) - 0.055
    )


def get_shape(shape):
    nsamples = 32
    resolution = 512

    image = oqmc.plot_shape(shape, nsamples, resolution)
    image = eotf_inverse_sRGB(image)
    image = np.clip(image, 0, 1)

    return image


def get_image(sequence, max_depth, num_pixel_samples=2):
    scene = b"box"
    width = 512
    height = 512
    frame = 0
    num_light_samples = 1
    max_opacity = 1
    exposure = 3

    image = oqmc.trace(
        sequence,
        scene,
        width,
        height,
        frame,
        num_pixel_samples,
        num_light_samples,
        max_depth,
        max_opacity,
    )
    image = image * exposure
    image = eotf_inverse_sRGB(image)
    image = np.clip(image, 0, 1)

    return image
