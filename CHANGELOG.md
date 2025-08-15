# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- SVG formated version of the logo to the images folder.

### Changed

- Update org name to AcademySoftwareFoundation.

### Deprecated
### Removed
### Fixed
### Security

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
