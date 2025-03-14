# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

{
  description = "Quasi-Monte Carlo sampling library for rendering and graphics applications";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/release-24.11";
    flake-utils.url = "github:numtide/flake-utils";
    hypothesis.url = "github:joshbainbridge/hypothesis";
    hypothesis.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = { self, nixpkgs, flake-utils, hypothesis }:
    flake-utils.lib.eachDefaultSystem (system:
      let pkgs = nixpkgs.legacyPackages.${system}; in {
        packages.default = pkgs.stdenv.mkDerivation {
          name = "openqmc";
          src = self;
          buildInputs = [
            pkgs.cmake
          ];
        };
        devShells.default = pkgs.mkShell.override {
          stdenv = pkgs.stdenvNoCC;
        } {
          name = "devshell";
          packages = [
            pkgs.git
            pkgs.cmakeCurses
            pkgs.llvm
            pkgs.clang-tools
            pkgs.clang
            pkgs.ninja
            pkgs.just
            pkgs.ruff
            pkgs.glm
            pkgs.doxygen
            pkgs.graphviz
            pkgs.tbb_2021_11
            # pkgs.gtest // Not compatible with C++ 14; falling back to FetchContent.
            hypothesis.packages.${system}.default
            (pkgs.python311.withPackages(ps: with ps; [
              python-lsp-server
              matplotlib
              numpy
              pillow
              jupyter
            ]))
          ];
          inputsFrom = [
            self.packages.${system}.default
          ];
          shellHook = ''
            export TOOLSPATH=$PWD/build/src/tools/lib
            export PYTHONPATH=$PYTHONPATH:$PWD/python
          '';
        };
      }
    );
}
