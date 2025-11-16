#!/usr/bin/env bash

# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

set -e

curl -L -O https://raw.githubusercontent.com/jothepro/doxygen-awesome-css/v2.4.0/doxygen-awesome.css
curl -L -O https://raw.githubusercontent.com/jothepro/doxygen-awesome-css/v2.4.0/doxygen-awesome-sidebar-only.css

doxygen
