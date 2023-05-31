# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

#!/bin/bash

export SAVEPLOT=light
jupyter execute python/notebooks/performance.ipynb python/notebooks/readme-images.ipynb

export SAVEPLOT=dark
jupyter execute python/notebooks/performance.ipynb python/notebooks/readme-images.ipynb
