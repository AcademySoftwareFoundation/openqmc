# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

{
  description = "Quasi-Monte Carlo sampling library for rendering and graphics applications";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/23.05";
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
            pkgs.clang-tools
            pkgs.clang
            pkgs.git
            pkgs.cmakeCurses
            pkgs.ninja
            pkgs.just
            pkgs.glm
            pkgs.gtest
            pkgs.tbb_2021_8
            hypothesis.packages.${system}.default
          ];
          inputsFrom = [
            self.packages.${system}.default
          ];
        };
        devShells.notebook = pkgs.mkShell.override {
          stdenv = pkgs.stdenvNoCC;
        } {
          name = "notebook";
          packages = [
            pkgs.python310Packages.python-lsp-server
            pkgs.python310Packages.python-lsp-ruff
            pkgs.python310Packages.jupyter
            pkgs.python310Packages.matplotlib
            pkgs.python310Packages.numpy
            pkgs.python310Packages.pillow
          ];
          inputsFrom = [
            self.devShells.${system}.default
          ];
          shellHook = ''
            export TOOLSPATH=$PWD/build/src/tools/lib
            export PYTHONPATH=$PYTHONPATH:$PWD/python
          '';
        };
      }
    );
}
