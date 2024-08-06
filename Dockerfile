# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

FROM nixpkgs/nix-flakes:nixos-24.05
RUN nix profile install nixpkgs#git-lfs && git lfs install
