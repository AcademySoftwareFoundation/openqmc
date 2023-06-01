# OpenQMC

[![License](https://img.shields.io/badge/license-Apache--2.0-informational)](https://github.com/framestore/openqmc/blob/main/LICENSE)
[![Version](https://img.shields.io/github/v/release/framestore/openqmc)](https://github.com/framestore/openqmc/releases/latest)
[![CI](https://github.com/framestore/openqmc/workflows/ci/badge.svg)](https://github.com/framestore/openqmc/actions?query=branch%3Amain)

OpenQMC is a small library for sampling high quality Quasi-Monte Carlo (QMC)
points and generating pseudo random numbers. It's geared towards graphics
applications, specifically rendering and film production. The library is a
variant of that developed at Framestore for their production renderer Freak.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/openqmc-example-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/openqmc-example-dark.png">
  <img alt="Comparing OpenQMC to RNG when rendering the cornell box." src="./images/plots/openqmc-example-light.png">
</picture>

## Description

This C++14 (CPU, GPU) header only library provides an API to deliver high
quality QMC sample points. The API aims to be compatible with a variety of
common use cases found in the wild, while maximising the rate of convergence.
It also provides multiple state of the art back-end implementations.

There are three primary aims for the project:

- Standardise a flexible sampler API.
- Deliver performant implementations.
- Provide high quality results.

The project doesn't aim to:

- Implement an exhaustive list of sequences.
- Provide educational implementations.

Project features are:

- Different architectures (CPU, GPU, etc).
- Static interface / zero cost abstraction.
- Solutions for different sampling use cases.
- Supports progressive / adaptive pixel sampling.
- Suitable for depth and wavefront rendering.
- Clear and extendable code base.
- Unit and statistical testing.
- Modern [CMake](https://cmake.org/) based build system.
- Header only and binary configurations.
- No library or STL dependencies.
- Includes tools and examples.

The project maintainers would like developers to fearlessly levage QMC sampling
evreywhere. This is what the OpenQMC API and its domain trees makes possible.
It allows you to focus on writing graphics technology, while safely and
automatically getting the best in class rates of convergence possible.

A variant of the library is actively used by Framestore's renderer
[Freak](https://www.framestore.com/work/rendering) to produce rich, feature
film VFX content. As a result the library has been battle tested in production
on a large scale. Framestore are commited to continue contributing any further
improvements from production to this open initiative.

## Requirements

The library itself has no dependencies other than C++14. Although not tested
with older versions of GCC, this should make it compatible with CY2018 and
newer versions of the [VFX Reference Platform](https://vfxplatform.com).

Supported operating systems:

- Linux
- macOS
- Windows (in development)

Supported architectures:

- x86-64
- AArch64
- NVIDIA

Tested compilers:

- Clang 6
- GCC 7.5
- Apple Clang 13
- NVCC 10

## Installation

You can install OpenQMC via either a package manager, or by cloning the source
and installing it directly. The former is easier to get up and going, but the
later will give you some build options to play with.

### NixOS package

OpenQMC is avaible as a Nix flake package on NixOS and other supported
platforms. You can also use the Nix flake as a hermetic dev environment:

```bash
nix develop github:framestore/openqmc
```

### Cloning the source

If you just need the library source and don't need any of the development tools
or tests, then you can ignore the submodules. You can clone the project with:

```bash
git clone git@github.com:framestore/openqmc.git
```

## Adding to your project

There are three methods for adding the library as an upstream dependency to
another project. Examples of each are in the [cmake/examples](cmake/examples)
directory. These are working examples and are continuously validated.

### Add library as a submodule

The first and most recommended method is to add the library as a submodule to
your downstream project, and then include the library using CMake. This is the
fastest and simplest approach.

```cmake
# Optionally set options (they are OFF by default)
set(OPENQMC_ARCH_TYPE AVX CACHE STRING "" FORCE)
set(OPENQMC_ENABLE_BINARY ON CACHE BOOL "" FORCE)
set(OPENQMC_SHARED_LIB ON CACHE BOOL "" FORCE)

# Load external dependencies
add_subdirectory(path/to/submodule EXCLUDE_FROM_ALL)

# Optionally mark options as advanced
mark_as_advanced(FORCE OPENQMC_ARCH_TYPE)
mark_as_advanced(FORCE OPENQMC_BUILD_TOOLS)
mark_as_advanced(FORCE OPENQMC_BUILD_TESTING)
mark_as_advanced(FORCE OPENQMC_USE_SUBMODULES)
mark_as_advanced(FORCE OPENQMC_ENABLE_BINARY)
mark_as_advanced(FORCE OPENQMC_SHARED_LIB)
mark_as_advanced(FORCE OPENQMC_FORCE_PIC)

# Add dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE OpenQMC::OpenQMC)
```

### Add library as a config-module

Another option is to install the library and use the same configuration across
multiple downstream projects. You may want to do this if the library is shared
between projects.

Installation from source is orthodox CMake. CMake defaults the target install
directory to `/usr/local` which can be overridden with `CMAKE_INSTALL_PREFIX`,
along with the other optional defines on the command line. An example would be:

```bash
cmake -B build -D CMAKE_INSTALL_PREFIX=/install/path -D OPENQMC_ENABLE_BINARY=ON
cmake --build build --target install
```

Adding the library then requires a `find_package` call in your downstream CMake
project. This uses a CMake config file, which the preceding command would have
installed, so the configuration is automatically loaded.

```cmake
# Find external dependencies
find_package(OpenQMC CONFIG REQUIRED)

# Optionally mark DIR variable as advanced
mark_as_advanced(OpenQMC_DIR)

# Add dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE OpenQMC::OpenQMC)
```

### Add library manually

Alternatively you can add the library manually as header only. Although doing
so prevents use of options and isn't recommended.

```cmake
# Enable C++14 support
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_14)

# Add dependencies
target_include_directories(${PROJECT_NAME} PRIVATE path/to/library/include)
```

### Library build options

When adding the library using CMake, there are a set of configuration options
which apply to downstream projects. The options are:

- `OPENQMC_ARCH_TYPE`: Sets the architecture to optimise for on the CPU, or to
  target NVIDIA GPUs. Option values can be `Scalar`, `SSE`, `AVX`, `ARM` or
  `GPU`. Default value is `Scalar`.
- `OPENQMC_ENABLE_BINARY`: You can reduce binary size of downstream projects by
  opting for a binary variant of the library. Option values can be `ON` or
  `OFF`. Default value is `OFF`.
- `OPENQMC_SHARED_LIB`: You can request a shared library instead of the default
  static. This also automatically enables PIC. Option values can be `ON` or
  `OFF`. Default value is `OFF`.
- `OPENQMC_FORCE_PIC`: When compiling a static library and the downstream
  project is a shared library, you can force enable PIC. Option values can be
  `ON` or `OFF`. Default value is `OFF`.

## Usage

This is an introduction to using the library. The example goes through how to
create an image of a quarter disk and anti-alias the pixels with supersampling.

The algorithm loops over all pixels and for each sample adds a small value if a
random pixel offset falls within the quarter disk. The end result is an image
of the disk with anti-aliased pixels across the edge of the shape.

If you implemented this using OpenQMC it might look like:

```cpp
// 1. Initialise the sampler cache.
auto cache = new char[oqmc::PmjSampler::cacheSize];
oqmc::PmjSampler::initialiseCache(cache);

// 2. Loop over all pixels in the image.
for(int x = 0; x < resolution; ++x)
{
	for(int y = 0; y < resolution; ++y)
	{
		// 3. Loop over all the sample indices.
		for(int index = 1; index < sampleSize; ++index)
		{
			// 4. Create a sampler object for the pixel domain.
			const auto domain = oqmc::PmjSampler(x, y, 0, index, cache);

			// 5. Draw a sample point from the domain.
			float samples[2];
			domain.drawSample<2>(samples);

			// 6. Offset the point into the pixel.
			const auto xOffset = samples[0] + x;
			const auto yOffset = samples[1] + y;

			// 7. Add value to the pixel if within disk.
			if(xOffset * xOffset + yOffset * yOffset < resolution * resolution)
			{
				image[x * resolution + y] += 1.0f / sampleSize;
			}
		}
	}
}

// 8. Deallocate the sampler cache.
delete[] cache;
```

There are more advanced examples ([path tracer](src/tools/lib/trace.cpp),
[table generator](src/tools/lib/generate.cpp), etc) under the
[src/tools/lib](src/tools/lib) directory. To learn more, have a look at the
[API reference](#api-reference) and [concepts](#concepts-and-examples) below.

## Versioning

Version numbers follow [Semantic Versioning](http://semver.org/) to indicate
how changes effect the API between releases of the library. Given a version
number MAJOR.MINOR.PATCH, incrementing a version means:

1. MAJOR indicates an incompatible API change.
2. MINOR indicates a backwards compatible feature addition.
3. PATCH indicates a backwards compatible bug fix.

Additional labels for pre-release and build metadata may be present as
extensions to the MAJOR.MINOR.PATCH format.

You can access details on what changed for each new version at
[CHANGELOG.md](CHANGELOG.md). This follows the format from [Keep a
Changelog](https://keepachangelog.com/en/1.0.0/).

## API reference

You have access to individual components used to build the samplers, and are
free to use these directly. But it's recommended that users take advantage of
the sampler interface which defines a generic API for all implementations, and
provides a contract to the calling code.

List of all sampler implementation header files:

- [`oqmc/oqmc.h`](include/oqmc/oqmc.h): Convenience header includes all implementations.
- [`oqmc/pmj.h`](include/oqmc/pmj.h): Includes low discrepancy `oqmc::PmjSampler`.
- [`oqmc/pmjbn.h`](include/oqmc/pmjbn.h): Includes blue noise variant `oqmc::PmjBnSampler`.
- [`oqmc/sobol.h`](include/oqmc/sobol.h): Includes Owen scrambled `oqmc::SobolSampler`.
- [`oqmc/sobolbn.h`](include/oqmc/sobolbn.h): Includes blue noise variant `oqmc::SobolBnSampler`.
- [`oqmc/lattice.h`](include/oqmc/lattice.h): Includes rank one `oqmc::LatticeSampler`.
- [`oqmc/latticebn.h`](include/oqmc/latticebn.h): Includes blue noise variant `oqmc::LatticeBnSampler`.

This is the sampler API reference, which is the generic interface for all
sampler implementations. As seen used in the [Usage](#usage) section.

```cpp
/**
 * @brief Required allocation size of the cache.
 * @details Prior to construction of a sampler object, a cache needs to be
 * allocated and initialised for any given sampler type. This variable is
 * the minimum required size in bytes of that allocation. The allocation
 * itself is performed and owned by the caller. Responsibility for the
 * de-allocation is also that of the caller.
 */
static std::size_t oqmc::Sampler::cacheSize;
```

```cpp
/**
 * @brief Initialise the cache allocation.
 * @details Prior to construction of a sampler object, a cache needs to be
 * allocated and initialised for any given sampler type. This function will
 * initialise that allocation. Once the cache is initialised it may be used
 * to construct a sampler object, or copied to a new address.
 *
 * Care must be taken to make sure the memory address is accessible at the
 * point of construction. On the CPU this is trivial. But when constructing
 * a sampler object on the GPU, the caller is expected to either use unified
 * memory for the allocation, or manually copy the memory from the host to
 * the device after the cache has been initialised.
 *
 * A single cache (for each sampler type) is expected to be constructed only
 * once for the duration of a calling process. This single cache can be used
 * to construct many sampler objects.
 *
 * @param [in, out] cache Memory address of the cache allocation.
 *
 * @pre Memory for the cache must be allocated prior. The minimum size of
 * the allocation can be retrieved using the `cacheSize` variable above.
 *
 * @post Memory for the cache must be de-allocated after all instances of
 * the sampler object have been destroyed.
 */
static void oqmc::Sampler::initialiseCache(void* cache);
```

```cpp
/**
 * @brief Parametrised pixel constructor.
 * @details Create a sampler object based on the pixel, frame and sample
 * indices. This also requires a pre-allocated and initialised cache. Once
 * constructed the sampler object is valid and ready for use.
 *
 * Values for sampleId should be within the [0, 2^16) range; this constraint
 * allows for possible optimisations in the implementations.
 *
 * @param x [in] Pixel coordinate on the x axis.
 * @param y [in] Pixel coordinate on the y axis.
 * @param frame [in] Time index value.
 * @param sampleId [in] Sample index. Must be within [0, 2^16).
 * @param cache [in] Allocated and initialised cache.
 *
 * @pre Cache has been allocated in memory accessible to the device calling
 * this constructor, and has also been initialised.
 */
oqmc::Sampler::Sampler(int x, int y, int frame, int sampleId, const void* cache);
```

```cpp
/**
 * @brief Derive an object in a new domain.
 * @details The function derives a mutated copy of the current sampler
 * object. This new object is called a domain. Each domain produces an
 * independent 4 dimensional pattern. Calling the draw* member functions
 * below on the new child domain will produce a different value to that of
 * the current parent domain.
 *
 * N child domains can be derived from a single parent domain with the use
 * of the key argument. Keys must have at least one bit difference, but can
 * be a simple incrementing sequence. A single child domain can itself
 * derive N child domains. This process results in a domain tree.
 *
 * The calling code can use up to 4 dimensions from each domain (these are
 * typically of the highest quality), joining them together to form an N
 * dimensional pattern. This technique is called padding.
 *
 * @param [in] key Index key of next domain.
 * @return Child domain based on the current object state and key.
 */
oqmc::Sampler oqmc::Sampler::newDomain(int key) const;
```

```cpp
/**
 * @brief Derive an object in a new domain for a local distribution.
 * @details Like newDomain, this function derives a mutated copy of the
 * current sampler object. The difference is it decorrelates the pattern
 * with the sample index, allowing for sample splitting with an unknown
 * multiplier.
 *
 * Calling code that needs to take N branching samples can't simply call the
 * nextDomainIndex member function shown below. This would cause duplication
 * with other domain trees, and ultimately bias the estimate. By deriving a
 * decorrelated domain using this member function, the calling code can
 * safely iterate N samples from the resulting child domain.
 *
 * The resulting pattern from taking multiple samples with nextDomainIndex
 * will be well distributed locally. But care must be taken as this local
 * pattern will be of lower quality when combined with the local patterns of
 * other samples. As the correlation between the patterns globally has been
 * lost. If the multiplier (the amount of times nextDomainIndex is to be
 * called) is known and constant, newDomainSplit can be used instead to
 * produce the best quality results.
 *
 * @param [in] key Index key of next domain.
 * @return Child domain based on the current object state and key.
 */
oqmc::Sampler oqmc::Sampler::newDomainDistrib(int key) const;
```

```cpp
/**
 * @brief Derive an object in a new domain for splitting.
 * @details Like the other newDomain* functions, this function derives a
 * mutated copy of the current sampler object. The difference is it remaps
 * the sample index, allowing for sample splitting with a known and constant
 * multiplier.
 *
 * Calling code that needs to take N branching samples can't simply call the
 * nextDomainIndex member function shown below. This would cause duplication
 * with other domain trees, and ultimately bias the estimate. By deriving a
 * new domain using this member function, the calling code can safely
 * iterate N samples from the resulting child domain.
 *
 * As the multiplier is a known and constant size, not only will the
 * resulting pattern from taking multiple samples with nextDomainIndex be
 * well distributed locally, but it will also be well distributed globally.
 * This is because the indices can be mapped more carefully when the size of
 * N is known and does not change. If these guarantees can not be met then
 * the calling code should use newDomainDistrib instead.
 *
 * @param [in] key Index key of next domain.
 * @param [in] size Sample index multiplier. Must be greater than zero.
 * @return Child domain based on the current object state, key and size.
 */
oqmc::Sampler oqmc::Sampler::newDomainSplit(int key, int size) const;
```

```cpp
/**
 * @brief Derive an object in the current domain at the next index.
 * @details The function derives a mutated copy of the current sampler
 * object. This new object is in the same domain, but will have iterated
 * onto the next sample index. Calling the draw* member functions below on
 * the new index will produce a different value to that of the previous
 * index.
 *
 * This is used to split a single sample index into multiple indices so that
 * a subset of an integrals dimensions can sampled at higher rate than other
 * dimensions. This technique is called sample or trajectory splitting.
 *
 * The function should only be called on domains that have been configured
 * for splitting, else it would cause duplication with other domain trees,
 * and ultimately bias the estimate. If newDomainDistrib was used, then the
 * only restriction is that the resulting sample index must not exceed the
 * global limit of 2^16. If newDomainSplit was used, then the calling code
 * must iterate with this function as many times as was specified in the
 * 'size' argument.
 *
 * @return Child sampler object based on current state.
 *
 * @pre The current object must be the result of calling either the
 * newDomainDistrib or newDomainSplit member functions.
 */
oqmc::Sampler oqmc::Sampler::nextDomainIndex() const;
```

```cpp
/**
 * @brief Draw integer sample values from domain
 * @details This can compute sample values with up to 4 dimensions for the
 * given domain. The operation does not change the state of the object, and
 * for a single domain and index, the result of this function will always be
 * the same. Output values are uniformly distributed integers within the
 * range of [0, 2^32).
 *
 * These values are of high quality and should be handled with care as to
 * not introduce bias into an estimate. For low quality, but fast and safe
 * random numbers, use the drawRnd member functions below.
 *
 * @tparam Size Number of dimensions to draw. Must be within [1, 4].
 *
 * @param [out] samples Output array to store sample values.
 */
template <int Size>
void oqmc::Sampler::drawSample(std::uint32_t samples[Size]) const;
```

```cpp
/**
 * @brief Draw floating point sample values from domain
 * @details This function wraps the integer variant of drawSample above. But
 * transforms the output values into uniformly distributed floats within the
 * range of [0, 1).
 *
 * @tparam Size Number of dimensions to draw. Must be within [1, 4].
 *
 * @param [out] samples Output array to store sample values.
 */
template <int Size>
void oqmc::Sampler::drawSample(float samples[Size]) const;
```

```cpp
/**
 * @brief Draw integer pseudo random values from domain
 * @details This can compute rnd values with up to 4 dimensions for the
 * given domain. The operation does not change the state of the object, and
 * for a single domain and index, the result of this function will always be
 * the same. Output values are uniformly distributed integers within the
 * range of [0, 2^32).
 *
 * These values are of low quality but are fast to compute and have little
 * risk of biasing an estimate. For higher quality samples, use the
 * drawSample member functions above.
 *
 * @tparam Size Number of dimensions to draw. Must be within [1, 4].
 *
 * @param [out] samples Output array to store sample values.
 */
template <int Size>
void oqmc::Sampler::drawRnd(std::uint32_t rnds[Size]) const;
```

```cpp
/**
 * @brief Draw floating point pseudo random values from domain
 * @details This function wraps the integer variant of drawRnd above. But
 * transforms the output values into uniformly distributed floats within the
 * range of [0, 1).
 *
 * @tparam Size Number of dimensions to draw. Must be within [1, 4].
 *
 * @param [out] samples Output array to store sample values.
 */
template <int Size>
void oqmc::Sampler::drawRnd(float rnds[Size]) const;
```

## Implementation details

The generic API described preceding is available from multiple implementations
which are interchangeable at compile time. Each implementation has different
strengths that balance performance and quality. This section provides a
comparison between implementations and details on each.

### Design overview

This diagram gives a high level view of the available implementations and how
they relate to one another. You can see that for each implementation there is
also a blue noise variant. More on this below.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/diagrams/high-level-overview-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/diagrams/high-level-overview-dark.png">
  <img alt="High level implementation diagram." src="./images/diagrams/high-level-overview-light.png">
</picture>

### Rate of convergence

These plots show the rate of convergence for different implementations across
different two dimensional integrals. Each plot takes the average of 128 runs,
plotting a data point for every sample count.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/error-plot-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/error-plot-dark.png">
  <img alt="Error plot comparison." src="./images/plots/error-plot-light.png">
</picture>

The following plots show, for a fixed sample count, how the error decreases
with a Gaussian filter as the standard deviation increases. It shows,
especially for low samples counts, the faster convergence of the blue noise
variants. This is an indicator that these may be a better option if your images
are then filtered with a denoise pass.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/error-filter-space-plot-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/error-filter-space-plot-dark.png">
  <img alt="Error filter space plot comparison." src="./images/plots/error-filter-space-plot-light.png">
</picture>

The preceding plots filtered the error spatially. The following plots filter
the error temporally. They show that averaging the frames over time also
favours the blue noise variants. Useful if you feed the frames into a temporal
anti-aliasing algorithm.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/error-filter-time-plot-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/error-filter-time-plot-dark.png">
  <img alt="Error filter time plot comparison." src="./images/plots/error-filter-time-plot-light.png">
</picture>

### Zoneplates

These plots show zoneplate tests for each sampler implementation, and give an
indication of any potential for structural aliasing across varying frequencies
and directions. Often this manifest as a moire pattern.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/zoneplate-plot-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/zoneplate-plot-dark.png">
  <img alt="Zoneplate test comparison." src="./images/plots/zoneplate-plot-light.png">
</picture>

### Performance

In these charts you can see the time it takes to compute N samples for each
implementation. Each sample takes 256 dimensions. It also shows the cost of
cache initialisation, this is often a trade off for runtime performance. The
plot measured the results on an Apple Silicon M1 Max with `OPENQMC_ARCH_TYPE`
set to `ARM` for Neon intrinsics.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/performance-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/performance-dark.png">
  <img alt="Performance comparison." src="./images/plots/performance-light.png">
</picture>

### Pixel sampling

This is the result of rendering the Cornell box with each implementation. There
are two rows of insets. The top row shows pixel variance due to a discontinuous
visibility term. And the bottom row shows the variance of sampling a light at a
grazing angle. The render for all insets had 2 samples per pixel. Numbers
indicate RMSE.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/cornell-box-0-2-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/cornell-box-0-2-dark.png">
  <img alt="Cornell box direct comparison." src="./images/plots/cornell-box-0-2-light.png">
</picture>

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/cornell-box-1-2-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/cornell-box-1-2-dark.png">
  <img alt="Cornell box indirect comparison." src="./images/plots/cornell-box-1-2-light.png">
</picture>

With higher sample counts per pixel (the results below are using 32 samples)
the difference between the sampler implementations becomes greater as the
higher rate of convergence takes effect.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/cornell-box-1-32-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/cornell-box-1-32-dark.png">
  <img alt="Cornell box many samples comparison." src="./images/plots/cornell-box-1-32-light.png">
</picture>

### Pmj sequences

The implementation uses the stochastic method described by Helmer et la. [^2]
to efficiently construct a progressive multi-jittered (0,2) sequence. The first
pair of dimensions in a domain have the same intergration properties as the
Sobol implementation. However as the sequence doesn't extend to more than two
dimensions, the second pair is randomised relative to the first in a single
domain.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/diagrams/pmj-design-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/diagrams/pmj-design-dark.png">
  <img alt="Pmj pair plot." src="./images/diagrams/pmj-design-light.png">
</picture>

This sampler pre-computes a base 4D pattern for all sample indices during the
cache initialisation. Permuted index values are then looked up from memory at
runtime, before being XOR scrambled. This amortises the cost of initialisation.
The rate of integration is very high, especially for the first and second pairs
of dimensions. You may however not want to use this implementation if memory
space or access is a concern.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/pair-plot-pmj-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/pair-plot-pmj-dark.png">
  <img alt="Pmj pair plot." src="./images/plots/pair-plot-pmj-light.png">
</picture>

### Sobol sequences

The implementations use an elegant construction by Burley [^1] for an Owen
scrambled Sobol sequence. This also includes performance improvements such as
limiting the index to 16 bits, pre-inverting the input and output matrices, and
making use of CPU vector intrinsics. You need to select an `OPENQMC_ARCH_TYPE`
to make use of the performance from vector intrinsics for a given architecture.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/diagrams/sobol-design-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/diagrams/sobol-design-dark.png">
  <img alt="sobol pair plot." src="./images/diagrams/sobol-design-light.png">
</picture>

This sampler has no cache initialisation cost, it generates all samples on the
fly without touching memory. However the cost per draw sample call is
computationally higher than other samplers. The quality of Owen scramble
sequences often outweigh this cost due to their random error cancellation and
incredibly high rate of integration for smooth functions.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/pair-plot-sobol-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/pair-plot-sobol-dark.png">
  <img alt="Sobol pair plot." src="./images/plots/pair-plot-sobol-light.png">
</picture>

### Lattice sequences

The implementations use the generator vector from Hickernell et al. [^3] to
construct a 4D lattice. This is then made into a progressive sequence using
a scalar based on a radical inversion of the sample index. Randomisation uses
toroidal shifts.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/diagrams/lattice-design-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/diagrams/lattice-design-dark.png">
  <img alt="lattice pair plot." src="./images/diagrams/lattice-design-light.png">
</picture>

This sampler has no cache initialisation cost, it generates all samples on the
fly without touching memory. Runtime performance is also high with a relatively
low computation cost for a single draw sample call. However the rate of
integration per pixel can be lower when compared to other samplers.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/pair-plot-lattice-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/pair-plot-lattice-dark.png">
  <img alt="Lattice pair plot." src="./images/plots/pair-plot-lattice-light.png">
</picture>

### Spatial temporal blue noise

The blue noise variants offer spatial temporal blue noise dithering between
pixels, with progressive ranking for progressive pixel sampling. This involves
an offline optimisation process that's based on the work by Belcour and Heitz
[^4], and extends temporally as described by Wolfe et al. [^5].

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/diagrams/optimise-index-seed-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/diagrams/optimise-index-seed-dark.png">
  <img alt="lattice pair plot." src="./images/diagrams/optimise-index-seed-light.png">
</picture>

Each variant achieves a blue noise distribution using two pixel tables, one
holds keys to seed the sequence, and the other index ranks. These tables are
then randomised by toroidally shifting the table lookups for each domain using
random offsets. Correlation between the offsets and the pixels allows for a
single pair of tables to provide keys and ranks for many domains.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/diagrams/blue-noise-procedure-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/diagrams/blue-noise-procedure-dark.png">
  <img alt="lattice pair plot." src="./images/diagrams/blue-noise-procedure-light.png">
</picture>

Although the spatial temporal blue noise doesn't reduce the error for an
individual pixel, it does give a better perceptual result due to less low
frequency structure in the error between pixels. Also if an image is either
spatially or temporally filtered (as with denoising or temporal anti-aliasing),
the resulting error can be much lower when using a blue noise variant as shown
in [Rate of convergence](#rate-of-convergence).

For more details, see the [Optimise CLI](#optimise-cli) section.

### Object state (MOVE ELSEWHERE TBD)

The intention of the API is to allow for object instances to be efficiently
copied on the stack. In doing so the objects can be easily packed and queued
during wavefront rendering. Allowing for this means that along with the
possible 64 bits to track a cache, the state that uniquely identifies the
current domain and index of a sampler needs to be small as well.

For this purpose, each implementation uses 64 bits. Along with an optional
cache pointer, that gives a maximum of 128 bits for the entire object. Creating
new states for each domain relies heavily on PCG-RXS-M-RX-32 from the PCG
family of PRNGs as described by O'Neill [^6].

Each implementation uses the LCG state transition when advancing a domain, but
then increases the quality of the lower order bits with a permutation prior to
drawing samples. This allows the implementation to efficiently create and skip
domains, while producing high quality random results upon drawing samples.

## Concepts and examples

OpenQMC aims to provide an API that makes using high quality samples easy and
safe when writing software with numerical integration. You have already seen a
very basic example of this in the [Usage](#usage) section. This section will go
into more detail on the concepts, and provide some more complex examples.

The section will build upon a simple example, introduce the conepts of domain
trees and sample splitting, and show some of the potential pit-falls OpenQMC
prevents by following some basic rules. After this, you should be able to
fearlessly make use of QMC samples in your own software.

### Basic sampling

When you estimate an integral using the Monte Carlo method, the high dimensional
space can often be partitioned into logical domains. In OpenQMC domains are an
important concept. Domains act as promises that can be converted into dimensions
upon request by the caller. They can also be chained to create new domains.

This example function tries to compute a pixel estimate. This estimate is made
up of two parts, each needing a domain. The first is a `camera` that takes a
single sample, and the second is a `material`, that takes two samples.

```cpp
Result estimatePixel(const oqmc::PmjSampler pixelDomain,
                     const CameraInterface& camera,
                     const MaterialInterface& material)
{
	enum Domains
	{
		Next,
	};

	// Derive 'cameraDomain' from 'pixelDomain' parameter.
	const auto cameraDomain = pixelDomain.newDomain(Domains::Next);

	// Derive 'materialDomain' from 'cameraDomain' variable.
	const auto materialDomain = cameraDomain.newDomain(Domains::Next);

	// Take a single sample for time to pass to 'camera'.
	float timeSamples[1];
	cameraDomain.drawSample<1>(timeSamples);

	// Take two samples for wi direction to pass to 'material'.
	float wiSamples[2];
	materialDomain.drawSample<2>(wiSamples);

	// Pass drawn samples to the interface an importance sampling.
	const auto cameraSample = camera.sample(timeSamples);
	const auto materialSample = material.sample(wiSamples);

	// Using resulting samples, compute pixel estimate.
	return sampleLightTransport(cameraSample, materialSample);
}
```

Here we started with an initial `pixelDomain`, and then from that derived a
`cameraDomain`, and then finally from that derived a `materialDomain`. Domains
can provide up to 4 dimensions each. If more dimensions are required, the caller
can always derive a new domain. This sequential dependency as a graph looks
like:

GRAPH HERE.

This example works, but isn't very flexible. What if a type that implements the
camera or material interfaces requires less or more samples than is provided by
the calling code. This can evidently be improved.

Computing domains is cheap, but drawing samples is expensive. For example, if
there was a new type that implemented the material interface but did not require
any samples, it could opt to not draw the samples and save on the compute cost.

```cpp
Result estimatePixel(const oqmc::PmjSampler pixelDomain,
                     const CameraInterface& camera,
                     const MaterialInterface& material)
{
	enum Domains
	{
		Next,
	};

	// Derive 'cameraDomain' from 'pixelDomain' parameter.
	const auto cameraDomain = pixelDomain.newDomain(Domains::Next);

	// Derive 'materialDomain' from 'cameraDomain' variable.
	const auto materialDomain = cameraDomain.newDomain(Domains::Next);

	// Pass domains directly to the interface to compute samples.
	const auto cameraSample = camera.sample(cameraDomain);
	const auto materialSample = material.sample(materialDomain);

	// Using samples, compute pixel estimate.
	return sampleLightTransport(cameraSample, materialSample);
}
```

Looking at this revised implementation, the domains are now passed through the
interfaces, allowing the types to decide whether to draw the samples or not.
Here are two example types that implement the MaterialInterface:

```cpp
MaterialSample DiffuseMaterial::sample(const oqmc::PmjSampler materialDomain)
{
	// Take two samples for wi direction to compute a MaterialSample object.
	float wiSamples[2];
	materialDomain.drawSample<2>(wiSamples);

	// Compute MaterialSample object using the drawn samples...
}

MaterialSample SpecularMaterial::sample(const oqmc::PmjSampler materialDomain)
{
	// Compute MaterialSample object without drawing samples...
}
```

In the case of the `DiffuseMaterial`, the type continues to draw samples from the
domain. But in the case of the `SpecularMaterial`, this type has opted to save on
the cost of drawing samples, as they are not needed.

The next sub-section will go into achieving the opposite, and take more samples
instead of less. This will introduce a potential danger, as well as a new
concept called domain trees that can be used to do this operation safely.

### Domain branching

Continuing from the previous example, the higher level function remains the
same, but there is now a foreshadowing comment.

```cpp
Result estimatePixel(const oqmc::PmjSampler pixelDomain,
                     const CameraInterface& camera,
                     const MaterialInterface& material)
{
	enum Domains
	{
		Next,
	};

	// Derive 'cameraDomain' from 'pixelDomain' parameter.
	const auto cameraDomain = pixelDomain.newDomain(Domains::Next);

	// WARNING: First call to newDomain from 'cameraDomain' variable.
	const auto materialDomain = cameraDomain.newDomain(Domains::Next);

	// Pass domains directly to the interface to compute samples.
	const auto cameraSample = camera.sample(cameraDomain);
	const auto materialSample = material.sample(materialDomain);

	// Using samples, compute pixel estimate.
	return sampleLightTransport(cameraSample, materialSample);
}
```

The warning in the comment draws attention to `cameraDomain` being used to
derive `materialDomain`. In isolation, this is not a problem, but can become
one. Here is an example of a type implementing the CameraInterface, with another
foreshadowing comment:

```cpp
CameraSample ThinLensCamera::sample(const oqmc::PmjSampler cameraDomain)
{
	enum Domains
	{
		Next,
	};

	// WARNING: Second call to newDomain from 'cameraDomain' parameter.
	const auto lensDomain = cameraDomain.newDomain(Domains::Next);

	// Derive 'timeDomain' from 'lensDomain' variable.
	const auto timeDomain = lensDomain.newDomain(Domains::Next);

	// Take two samples for lens to compute a CameraSample object.
	float lensSamples[2];
	lensDomain.drawSample<2>(lensSamples);

	// Take a single sample for time to compute a CameraSample object.
	float timeSamples[1];
	timeDomain.drawSample<1>(timeSamples);

	// Compute CameraSample object using the drawn samples...
}
```

Here is where the problem lies... TBD.

### Fixed sample splitting

Example of fixed sample splitting, diagrams, etc.

### Dynamic sample splitting

Example of dynamic sample splitting, diagrams, etc.

## Development roadmap

Roadmap for version 1.0.0 of the library:

- Add complete documentation.
- Add support for Windows OS.

## Developer workflow

Beyond the library itself there are a range of tools and tests that make up the
project as a whole. These are there to aid development and to give an element
of analysis. These extra components also have more requirements and
dependencies than the library alone.

### Installing LFS

Some of the files in the repository require Git Large File Storage which is a
common Git extension for handling large file types. You can find installation
instructions on the [LFS](https://git-lfs.github.com) website.

### Dependencies

The tools and tests have dependencies on external projects. These can be either
installed on the system and automatically found, or alternatively cloned and
built within the project using submodules.

The dependencies are:

- [GLM](https://github.com/g-truc/glm)
- [GoogleTest](https://github.com/google/googletest)
- [Hypothesis](https://github.com/joshbainbridge/hypothesis)
- [oneTBB](https://github.com/oneapi-src/oneTBB)

### Cloning submodules

If a version of each of these dependencies is already installed on the system,
CMake can automatically find them. This means a basic clone of this repository
is sufficient as shown in [Cloning the source](#cloning-the-source).

You can alternatively clone and build the dependencies locally within the
project using submodules. If there is an installed version present on the
system this takes precedence.

If you haven't yet cloned the project and would like to do so with the
submodules use:

```bash
git clone --recursive git@github.com:framestore/openqmc.git
```

If you have already cloned the project and would like to add the submodules
retroactively then just run:

```bash
git submodule --init --recursive
```

### Extra build options

There are a few extra options that go along with those listed in the [Library
build options](#library-build-options) section. These are specific to handling
the tools and tests. The options are:

- `OPENQMC_BUILD_TOOLS`: Enable targets for building the project tools. You may
  want to enable during development. Option values can be `ON` or `OFF`.
  Default value is `OFF`.
- `OPENQMC_BUILD_TESTING`: Enable targets for building the project tests. You
  may want to enable during development. Option values can be `ON` or `OFF`.
  Default value is `OFF`.
- `OPENQMC_USE_SUBMODULES`: Force use of submodules even if dependencies are
  installed. Useful for guaranteeing compatibility. Option values can be `ON`
  or `OFF`. Default value is `OFF`.

### Build configuration

You can use configurations for [Extra build options](#extra-build-options)
based on your operating system by referencing pre-defined CMake presets. The
preset for Linux and macOS has the name `unix`. Presets initialise the build
config so that it's ready for development. You just need to run:

```bash
cmake --preset unix
```

You can also override the options using the CMake CLI while initialising the
build config. If the build already exists, then CMake updates the option:

```bash
cmake --preset unix -D OPENQMC_USE_SUBMODULES=ON
```

If you prefer to update the options using an interactive TUI rather than the
command line, then there is also the CMake curses interface command:

```bash
ccmake --preset unix
```

### Tools

The project comes with various tools that allow for offline optimisation of the
sampler implementations, as well as a way of evaluating the library results. A
benefit is that these tools also act as an example for users.

You can find the tools under the [src/tools](src/tools) folder. These make use
of the GLM and oneTBB external dependencies. You can build the tools using:

```bash
cmake --build --preset unix
```

Alternatively you can build a specific tool by specifying a target name:

```bash
cmake --build --preset unix --target benchmark
```

All tools link into a single library with a C API. You can find the source code
for this in [src/tools/lib](src/tools/lib) folder. Here is a list of some of
the key source files:

- [`src/tools/lib/benchmark.cpp`](src/tools/lib/benchmark.cpp) : Measure sampler performance.
- [`src/tools/lib/generate.cpp`](src/tools/lib/generate.cpp): Generate sample value tables.
- [`src/tools/lib/trace.cpp`](src/tools/lib/trace.cpp): Render a path traced image.
- [`src/tools/lib/optimise.cpp`](src/tools/lib/optimise.cpp): Run a blue noise optimisation.

You can access the library's C API via a Python CTypes wrapper. On Unix this
just requires the `LD_LIBRARY_PATH` to point towards the shared library binary
and importing the [python/wrapper.py](python/wrapper.py) module.

This library is also linked against multiple CLI commands found in the
[src/tools/cli](src/tools/cli) folder. Once you have compiled the tools, you
can run the CLI binaries from the command line.

#### Benchmark CLI

The `benchmark` tool measures the time for cache initialisation, as well as the
draw sample time, independently for each implementation. The results depend on
the hardware, as well as the build configuration.

Arguments are:

- `<sampler>`: options are `pmj`, `pmjbn`, `sobol`, `sobolbn`, `lattice`, `latticebn`.
- `<measurement>`: options are `init`, `samples`.

Command is:

```bash
./build/src/tools/cli/benchmark <sampler> <measurement>
```

#### Generate CLI

The `generate` tool evaluates a specific implementation and outputs a table of
points from the sequence. Currently the CLI sets the table output options to 2
sequences, 256 samples, and 8 dimensions.

Arguments are:

- `<sampler>`: options are `pmj`, `sobol`, `lattice`.

Command is:

```bash
./build/src/tools/cli/generate <sampler>
```

#### Trace CLI

The `trace` tool is a CPU / GPU uni-directional path tracer that provides an
example of how you might use the API in a renderer. There are multiple scenes
embedded in the source code, including the Cornell box used on this page. This
also demonstrates how each sampler implementation practically performs.

Arguments are:

- `<sampler>`: options are `pmj`, `pmjbn`, `sobol`, `sobolbn`, `lattice`, `latticebn`.
- `<scene>`: options are `box`, `presence`, `blur`.

Command is:

```bash
./build/src/tools/cli/trace <sampler> <scene>
```

#### Optimise CLI

The `optimise` tool targets a single base sampler implementation and produces
the data needed to construct a spatial temporal blue noise variant. The output
is two data tables. A keying table and a ranking table.

The tables are 3D with the axes representing pixel coordinates and frame. The
optimisation process works toroidally, so you can tile each table to cover a
large coordinate and temporal space.

Keys are 32 bit integers, and seed unique sampler domains. Ranks are also 32
bit integers, and when XORed with the sample index allow for spatial temporal
blue noise with progressive and adaptive rendering.

When optimising final quality results it's advisable to use a GPU build of the
optimiser due to the computational cost. Testing with an NVIDIA RTX A6000 found
that this provided a speedup of ~400x that of the CPU.

Arguments are:

- `<sampler>`: options are `pmj`, `sobol`, `lattice`.

Command is:

```bash
./build/src/tools/cli/optimise <sampler>
```

### Testing

The project aims to maintain full test coverage for all public library code.
That includes unit tests for individual functions and components. It also means
statistical hypothesis testing which validates the correctness of random
distributions.

You can find the tests under the [src/tests](src/tests) folder. These make use
of the GoogleTest and Hypothesis external dependencies. You can build the tests
using:

```bash
cmake --build --preset unix --target tests
```

Tests are then managed and run using CTest. You can run the tests using the
CTest CLI from the project root directory. Just run the command:

```bash
ctest --preset unix
```

CTest then aggregates the results of all tests from the GoogleTest framework
and writes the results to the terminal. These also include the null-hypothesis
tests that build upon Wenzel Jackob's
[hypothesis](https://github.com/wjakob/hypothesis) library.

### Running commands

If all those CMake commands sound laborious, there is an easier way, and that
is using a command runner like [just](https://github.com/casey/just). You can
find instructions to install just on their site.

You call a command in just a 'recipe', and these recipes simplify the commands
in the above documentation. If you run the just command without any arguments
from the project root directory, it will display all available recipes with
a description:

```bash
just
```

As a developer, you can setup the project using the `setup` recipe. This will
run the commands described in [Build configuration](#build-configuration) using
the `unix` preset:

```bash
just setup
```

By default just asks CMake to use [Ninja](https://ninja-build.org) as the build
generator. Instructions on getting Ninja are on their site. If you would like
to use a different generator, you can pass a parameter to `setup`:

```bash
just setup `Unix Makefiles`
```

After this point you can use any of the other recipes just lists, some of
these take arguments as you saw with `setup` and should simplify the commands
listed in the [Build configuration](#build-configuration), [Tools](#tools) and
[Testing](#testing) sections.

Here is an example of a recipe building and running the tests:

```bash
just test
```

### Notebooks

All Jupyter notebooks used to author the images on this page are under the
[python/notebooks](python/notebooks) directory and are available to see
directly online. These use the tools Python CTypes wrapper.

## Related projects

Here are details on other related projects, and how OpenQMC is different. If
OpenQMC doesn't solve your problem, one of these might be a better option:

- [Uni{corn|form} toolkit](https://github.com/utk-team/utk) is a great resource,
  but is more inteded for research rather than production use.
- [Andrew Helmer](https://github.com/Andrew-Helmer/stochastic-generation) gives
  a great example implementation of a their sampler, but no general interface.
- [Pbrt](https://github.com/mmp/pbrt-v4) is a seminal resource on all things
  rendering, but is written for educational purposes.

## Contributing

All contributions to the project are welcome. For guidance on how to contribute
please see the following pages:

- [CONTRIBUTING.md](CONTRIBUTING.md), process and procedures for contributing
  work towards the project. This includes signoff instructions, coding style,
  as well as other requirements for a pull request.
- [GOVERNANCE.md](GOVERNANCE.md), overview of the governing procedure, and
  the difference between project maintainer roles. The process of becoming a
  maintainer is outlined here.
- [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md), covers the code of conduct that we
  are commited to upholding as participants of the project's development, and
  as members of the larger community.

If instead you want to submit an issue or a request, report it by creating a
new GitHub [issue](https://github.com/framestore/openqmc/issues).

[^1]: Brent Burley. 2020.  
Practical Hash-based Owen Scrambling. JCGT.  
https://jcgt.org/published/0009/04/01/

[^2]: Andrew Helmer, Per H. Christensen and Andrew E. Kensler. 2021.  
Stochastic Generation of (t, s) Sample Sequences. EGSR.  
https://dx.doi.org/10.2312/sr.20211287

[^3]: Fred J. Hickernell, Peter Kritzer, Frances Y. Kuo and Dirk Nuyens. 2011.  
Weighted compound integration rules with higher order convergence for all N. Numerical Algorithms 59.  
https://dx.doi.org/10.1007/s11075-011-9482-5

[^4]: Laurent Belcour and Eric Heitz. 2021.  
Lessons Learned and Improvements when Building Screen-Space Samplers with Blue-Noise Error Distribution. ACM SIGGRAPH 2021 Talks.  
https://dx.doi.org/10.1145/3450623.3464645

[^5]: Alan Wolfe, Nathan Morrical, Tomas Akenine-Möller and Ravi Ramamoorthi. 2022.  
Spatiotemporal Blue Noise Masks. EGSR.  
https://dx.doi.org/10.2312/sr.20221161

[^6]: Melissa E. O'Neill. 2014.  
PCG: A Family of Simple Fast Space-Efficient Statistically Good Algorithms for Random Number Generation.  
https://www.pcg-random.org
