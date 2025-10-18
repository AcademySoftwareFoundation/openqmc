# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

{
  description = "Quasi-Monte Carlo sampling library for rendering and graphics applications";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
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
          stdenv = pkgs.clangStdenv;
        } {
          name = "devshell";
          packages = [
            pkgs.llvmPackages_18.clang-tools # matching version on GitHub runner
            pkgs.cmakeCurses
            pkgs.ninja
            pkgs.just
            pkgs.git
            pkgs.ty
            pkgs.ruff
            pkgs.doxygen
            pkgs.graphviz
            pkgs.glm
            pkgs.tbb
            pkgs.gtest
            pkgs.python313
            pkgs.python313Packages.mkdocs-material
            pkgs.python313Packages.matplotlib
            pkgs.python313Packages.numpy
            pkgs.python313Packages.pillow
            pkgs.python313Packages.notebook
            hypothesis.packages.${system}.default
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
