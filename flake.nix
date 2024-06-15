# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

{
  description = "Quasi-Monte Carlo sampling library for rendering and graphics applications";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/23.11";
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
            pkgs.cmakeCurses
            pkgs.llvm
            pkgs.clang-tools
            pkgs.clang
            pkgs.ninja
            pkgs.just
            pkgs.ruff
            pkgs.git
            pkgs.glm
            pkgs.gtest
            pkgs.doxygen
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
            (pkgs.python311.withPackages(ps: with ps; [
              python-lsp-server
              python-lsp-black
              python-lsp-ruff
              matplotlib
              numpy
              pillow
              jupyter
            ]))
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
