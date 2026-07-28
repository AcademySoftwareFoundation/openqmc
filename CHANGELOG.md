# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- GPU coverage on the CI pipeline is restored using ASWF hosted T4 runners, building the tools as CUDA and validating device results against CPU reference data.

### Changed

- Migrated the project from C++14 to C++17, moving the minimum supported VFX Reference Platform from CY2018 to CY2021, and the minimum NVCC version from 10 to 11. Blue noise tables are now `inline constexpr`, so header-only installs no longer duplicate table data across compilation units.
- Sped up scalar paths of `oqmc::sobolReversedIndex` using the construction method from Ahmed 2024.

### Deprecated
### Removed

- Removed the binary library variant and its `OPENQMC_ENABLE_BINARY`, `OPENQMC_SHARED_LIB` and `OPENQMC_FORCE_PIC` CMake options. C++17 `inline` variables make the header-only install equivalent, so downstream projects using these options should simply remove them.
- Removed the `oqmc/unused.h` header and its `OQMC_MAYBE_UNUSED` macro, which existed only because C++14 had no standard way to mark a symbol as possibly unused. Downstream projects using it should switch to the C++17 `[[maybe_unused]]` attribute.

### Fixed

- Improve quality of uniform float distribution in `oqmc::uintToFloat`.

### Security

## [0.7.1] - 2025-08-23

### Added

- SVG formated version of the logo to the images folder.

### Changed

- Update org name to AcademySoftwareFoundation.
- Add back per-frame randomization to sampler state.

## [0.7.0] - 2025-03-15

### Added

- Documentation and comments to track the generation and use of Sobol matrices.

### Fixed

- Docker developement environments now correctly forward the port for Jupyter Notebooks.

### Removed

- Temporal axis from blue noise variants, moving from STBN to SBN methods.

## [0.6.0] - 2024-08-04

### Added

- New `oqmc::SamplerInterface::newDomainChain()` function is a shorthand for two `oqmc::SamplerInterface::newDomain()`.
- Support and documentation for using Docker Compose to set up a development environment.

## [0.5.0] - 2024-05-05

### Added

- Initial support for Microsoft Windows OS has been added and is now supported on the CI. Details in the documentation.
- New integer range based variants of both `oqmc::SamplerInterface::drawSample()` and `oqmc::SamplerInterface::drawRnd()`.

### Changed

- Samplers no longer have an upper index limit of 2^16 and can take any positive integer on construction or when splitting.
- Simplified the API, updating `oqmc::SamplerInterface::newDomainDistrib()` and `oqmc::SamplerInterface::newDomainSplit()`.

### Removed

- Due to the changes above, the `oqmc::SamplerInterface::nextDomainIndex()` has been removed. Usage info in the documentation.

## [0.4.0] - 2024-01-30

### Changed

- Improvements to the trace tool, aimed at making the API calls clearer when used as an example.

## [0.3.0] - 2022-11-30

### Added

- Support default construction in the sampler interface to allow for placeholder objects.

## [0.2.0] - 2022-11-22

### Changed

- Respect LIBDIR CMake setting when installing CMake config files.
