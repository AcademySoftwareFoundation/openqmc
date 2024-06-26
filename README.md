<p align="center">
  <picture>
    <source media="(prefers-color-scheme: light)" srcset="./images/openqmc-logo-light.png">
    <source media="(prefers-color-scheme: dark)" srcset="./images/openqmc-logo-dark.png">
    <img alt="OpenQMC" src="./images/openqmc-logo-light.png" height="200">
  </picture>
</p>

[![License](https://img.shields.io/badge/License-Apache--2.0-informational)](https://github.com/framestore/openqmc/blob/main/LICENSE)
[![Release](https://img.shields.io/github/v/release/framestore/openqmc?label=Release)](https://github.com/framestore/openqmc/releases/latest)
![Coverage](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/joshbainbridge/c64d4efeaa4f0760255cc54cdadce85c/raw/test.json)
[![CI Pipeline](https://github.com/framestore/openqmc/actions/workflows/ci-pipeline.yml/badge.svg)](https://github.com/framestore/openqmc/actions/workflows/ci-pipeline.yml)

OpenQMC is a library for sampling high quality Quasi-Monte Carlo (QMC) points
and generating pseudo random numbers. Designed for graphics applications,
the library is part of Framestore's proprietary renderer [Freak](https://www.framestore.com/technology/freak)
and is actively used in VFX production.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/openqmc-example-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/openqmc-example-dark.png">
  <img alt="Comparing OpenQMC to RNG when rendering the cornell box." src="./images/plots/openqmc-example-light.png">
</picture>

## Description

This C++14 (CPU, GPU) header only library provides an API to deliver high
quality QMC sample points. The API aims to be compatible with a variety of
common use cases found in production codebases. The library also provides
multiple state of the art back-end implementations to minimise noise.

There are three primary aims for the project:

- Standardise a flexible sampler API.
- Deliver performant implementations.
- Provide high quality results.

The project doesn't aim to:

- Implement an exhaustive list of sequences.
- Provide educational implementations.

Project features are:

- Supports different architectures (CPU, GPU, etc).
- Static interface / zero cost abstraction.
- Solutions for different sampling use cases.
- Supports progressive / adaptive pixel sampling.
- Suitable for depth and wavefront rendering.
- Spatial temporal blue noise dithering.
- Clear and extendable code base.
- Unit and statistical testing.
- Modern [CMake](https://cmake.org/) based build system.
- Header only and binary configurations.
- No library or STL dependencies.
- Includes tools, docs and examples.

The API lets you focus on writing graphics algorithms while getting bias-free
results and best-in-class convergence rates. The project maintainers would
like developers to leverage QMC sampling everywhere confidently — this is what
the OpenQMC API and the concept of [domain branching](#domain-branching) aim
to achieve.

The project is battle-tested. Framestore actively uses a variant of this library
to produce visually rich feature film VFX content and is committed to continue
contributing any further improvements to this open initiative.

## Usage

Here is a quick example of what OpenQMC looks like. Feel free to copy and paste
this code to get yourself started, or continue reading to learn about other
samplers, the API, and techniques for writing more advanced algorithms.

```cpp
// 1. Include the sampler implementation.
#include <oqmc/pmjbn.h>

// 2. Initialise the sampler cache.
auto cache = new char[oqmc::PmjBnSampler::cacheSize];
oqmc::PmjBnSampler::initialiseCache(cache);

// 3. Loop over all pixels in the image.
for(int x = 0; x < resolution; ++x)
{
	for(int y = 0; y < resolution; ++y)
	{
		// 4. Loop over all the sample indices.
		for(int index = 0; index < nSamples; ++index)
		{
			// 5. Create a sampler object for the pixel domain.
			const auto domain = oqmc::PmjBnSampler(x, y, 0, index, cache);

			// 6. Draw a sample point from the domain.
			float sample[2];
			domain.drawSample<2>(sample);

			// 7. Offset the point into the pixel.
			const auto xOffset = sample[0] + x;
			const auto yOffset = sample[1] + y;

			// 8. Add value to the pixel if within disk.
			if(xOffset * xOffset + yOffset * yOffset < resolution * resolution)
			{
				image[x * resolution + y] += 1.0f / nSamples;
			}
		}
	}
}

// 9. Deallocate the sampler cache.
delete[] cache;
```

This algorithm loops over all pixels and for each sample adds a small value if a
random pixel offset falls within a quarter disk. The end result is an image of a
disk with anti-aliased pixels across the edge of the shape.

If you would like to see a real example of a path tracer, have a look at the
source code for the [trace](src/tools/lib/trace.cpp) tool. Or alternatively go
to the [concepts and examples](#concepts-and-examples) section to read about
more advanced solutions.

## Requirements

The library itself has no dependencies other than C++14. Although not tested
with older versions of GCC, this should make it compatible with CY2018 and
newer versions of the [VFX Reference Platform](https://vfxplatform.com).

Supported operating systems:

- Linux
- macOS
- Windows

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

OpenQMC is available as a Nix flake package on NixOS and other supported
platforms. The flake can also be used to load a reproducible [developer
environment](#nix-development-environment). Install the package with:

```bash
nix profile install github:framestore/openqmc
```

### Cloning the source

Alternatively for installing from source, you can download a release from the
site, or clone the project with:

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
mark_as_advanced(FORCE OPENQMC_FORCE_DOWNLOAD)
mark_as_advanced(FORCE OPENQMC_ENABLE_BINARY)
mark_as_advanced(FORCE OPENQMC_SHARED_LIB)
mark_as_advanced(FORCE OPENQMC_FORCE_PIC)

# Add dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE OpenQMC::OpenQMC)
```

### Add library as a config-module

Another good option is to install the library and use the same configuration
across multiple downstream projects. You may want to do this if the library is
shared between projects.

If you haven't used a package manager before or want to avoid the default
configuration, then you will need to install it from the source. The CMake
installation process will follow an idiomatic approach that provides you the
opportunity to set library build options. An example:

```bash
cmake -B build -D CMAKE_INSTALL_PREFIX=/install/path -D OPENQMC_ENABLE_BINARY=ON
cmake --build build --target install
```

Adding the library then requires a `find_package` call in your downstream CMake
project. `find_package` will access a CMake config file which was created upon
installation, allowing the installed requirements to be loaded automatically.

```cmake
# Find external dependencies
find_package(OpenQMC CONFIG REQUIRED)

# Optionally mark DIR variable as advanced
mark_as_advanced(OpenQMC_DIR)

# Add dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE OpenQMC::OpenQMC)
```

### Add library manually

Alternatively you can add the library manually as header-only. However, this
method isn't recommended, as it can be error-prone and prevents using build
options otherwise available via CMake.

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

## Versioning

Version numbers follow [Semantic Versioning](http://semver.org/) to indicate
how changes affect the API between releases of the library. Given a version
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

Down stream projects have access to individual components used to build the
samplers, and are free to use these directly. But it's recommended that users
take advantage of the sampler interface which defines a generic API for all
implementations, and provides a contract to the calling code.

The API is a static (compile-time polymorphic) interface. This is an important
detail, as it allows for implementations to be passed-by-value without dynamic
memory allocation, while also allowing inlining for zero-cost abstraction.

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
 * For each pixel this constructor is expected to be called multiple times,
 * once for each index. Pixels might have a varying number of indicies when
 * adaptive sampling.
 *
 * @param [in] x Pixel coordinate on the x axis.
 * @param [in] y Pixel coordinate on the y axis.
 * @param [in] frame Time index value.
 * @param [in] index Sample index. Must be positive.
 * @param [in] cache Allocated and initialised cache.
 *
 * @pre Cache has been allocated in memory accessible to the device calling
 * this constructor, and has also been initialised.
 */
oqmc::Sampler::Sampler(int x, int y, int frame, int index, const void* cache);
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
 * with the sample index, allowing for sample splitting with a non-constant
 * or unknown multiplier.
 *
 * The result from taking N indexed domains with this function will be a
 * locally well distributed sub-pattern. This sub-pattern will be of lower
 * quality when combined with the sub-patterns of other samples. That is
 * because the correlation between the sub-patterns globally is lost.
 *
 * If a mutliplier is known and constant then newDomainSplit will produce
 * better quality sample points and should be used instead. This is because
 * newDomainSplit will preserved correlation between sub-patterns from other
 * samples.
 *
 * Calling code should use a constant key for any given domain while then
 * incrementing the index value N times to increase the sampling rate by N
 * for that given domain. The function will be called N times, once for each
 * unique index.
 *
 * @param [in] key Index key of next domain.
 * @param [in] index Sample index of next domain. Must be positive.
 * @return Child domain based on the current object state and key.
 */
oqmc::Sampler oqmc::Sampler::newDomainDistrib(int key, int index) const;
```

```cpp
/**
 * @brief Derive an object in a new domain for global splitting.
 * @details Like the other newDomain* functions, this function derives a
 * mutated copy of the current sampler object. The difference is it remaps
 * the sample index, allowing for sample splitting with a known and constant
 * multiplier.
 *
 * The result from taking N indexed domains with this function will be both
 * a locally and a gloablly well distributed sub-pattern. This sub-pattern
 * will of the highest quality due to being globally correlated with the
 * sub-patterns of other samples.
 *
 * If a mutliplier is non-constant or unknown then newDomainDistrib should
 * be used instead. This is because newDomainDistrib relaxes the constraints
 * in excahnge for a reduction in the global quality of the pattern.
 *
 * Calling code should use a constant key for any given domain while then
 * incrementing the index value N times to increase the sampling rate by N
 * for that given domain. The function will be called N times, once for each
 * unique index. N must be passed as 'size' and remain constant.
 *
 * @param [in] key Index key of next domain.
 * @param [in] size Sample index multiplier. Must greater than zero.
 * @param [in] index Sample index of next domain. Must be positive.
 * @return Child domain based on the current object state, key and size.
 */
oqmc::Sampler oqmc::Sampler::newDomainSplit(int key, int size, int index) const;
```

```cpp
/**
 * @brief Draw integer sample values from domain.
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
 * @param [out] sample Output array to store sample values.
 */
template <int Size>
void oqmc::Sampler::drawSample(std::uint32_t sample[Size]) const;
```

```cpp
/**
 * @brief Draw ranged integer sample values from domain.
 * @details This function wraps the integer variant of drawSample above. But
 * transforms the output values into uniformly distributed integers within
 * the range of [0, range).
 *
 * @tparam Size Number of dimensions to draw. Must be within [1, 4].
 *
 * @param [in] range Exclusive end of range. Greater than zero.
 * @param [out] sample Output array to store sample values.
 */
template <int Size>
void oqmc::Sampler::drawSample(std::uint32_t range, std::uint32_t sample[Size]) const;
```

```cpp
/**
 * @brief Draw floating point sample values from domain.
 * @details This function wraps the integer variant of drawSample above. But
 * transforms the output values into uniformly distributed floats within the
 * range of [0, 1).
 *
 * @tparam Size Number of dimensions to draw. Must be within [1, 4].
 *
 * @param [out] sample Output array to store sample values.
 */
template <int Size>
void oqmc::Sampler::drawSample(float sample[Size]) const;
```

```cpp
/**
 * @brief Draw integer pseudo random values from domain.
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
 * @param [out] rnd Output array to store rnd values.
 */
template <int Size>
void oqmc::Sampler::drawRnd(std::uint32_t rnd[Size]) const;
```

```cpp
/**
 * @brief Draw ranged integer pseudo random values from domain.
 * @details This function wraps the integer variant of drawRnd above. But
 * transforms the output values into uniformly distributed integers within
 * the range of [0, range).
 *
 * @tparam Size Number of dimensions to draw. Must be within [1, 4].
 *
 * @param [in] range Exclusive end of range. Greater than zero.
 * @param [out] rnd Output array to store rnd values.
 */
template <int Size>
void oqmc::Sampler::drawRnd(std::uint32_t range, std::uint32_t rnd[Size]) const;
```

```cpp
/**
 * @brief Draw floating point pseudo random values from domain.
 * @details This function wraps the integer variant of drawRnd above. But
 * transforms the output values into uniformly distributed floats within the
 * range of [0, 1).
 *
 * @tparam Size Number of dimensions to draw. Must be within [1, 4].
 *
 * @param [out] rnd Output array to store rnd values.
 */
template <int Size>
void oqmc::Sampler::drawRnd(float rnd[Size]) const;
```

## Implementation comparison

The API is common to all back-end implementations. Each implementation has
different strengths that balance performance and quality. These are all the
implementations and their required header files:

- [`oqmc/pmj.h`](include/oqmc/pmj.h): Includes low discrepancy `oqmc::PmjSampler`.
- [`oqmc/pmjbn.h`](include/oqmc/pmjbn.h): Includes blue noise variant `oqmc::PmjBnSampler`.
- [`oqmc/sobol.h`](include/oqmc/sobol.h): Includes Owen scrambled `oqmc::SobolSampler`.
- [`oqmc/sobolbn.h`](include/oqmc/sobolbn.h): Includes blue noise variant `oqmc::SobolBnSampler`.
- [`oqmc/lattice.h`](include/oqmc/lattice.h): Includes rank one `oqmc::LatticeSampler`.
- [`oqmc/latticebn.h`](include/oqmc/latticebn.h): Includes blue noise variant `oqmc::LatticeBnSampler`.
- [`oqmc/oqmc.h`](include/oqmc/oqmc.h): Convenience header includes all implementations.

This diagram gives a high level view of the available implementations and how
they relate to one another. Each implementation includes a blue noise variant
which you can read more about [here](#spatial-temporal-blue-noise).

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/diagrams/high-level-overview-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/diagrams/high-level-overview-dark.png">
  <img alt="High level implementation diagram." src="./images/diagrams/high-level-overview-light.png">
</picture>

The following subsections will provide some analysis on each of the
implementations and outline their tradeoffs, so that you can make an informed
decision on what implementation would be best for your use case.

### Rate of convergence

*Unreasonable Effectiveness of QMC* is a subject worth exploring. When
discussing rates of convergence between different implementations, it can be
helpful to have some context in how these relate to classic MC. The primary
reason to use a QMC method in place of MC is the improvement it brings in
convergence rate, and as a result a reduction in computational cost. The
following figures illustrate this using an owen scrambled sobol sequence.

[quality (SE) vs cost] [N vs effective N] [Gaussian] [rate MC vs QMC].

Asymptotic complexity for a **Monte Carlo** (MC) estimator of some function $f$
typically looks like:

$$
Q_{N}\equiv\frac{1}{N}\sum_{i=1}^{N}f(x_i)
$$

$$
\mathcal{O}\left(N^{-0.5}\right)
\quad\text{or}\quad
\mathcal{O}\left(\frac{N^{0.5}}{N}\right)
$$

While both these formulas are equivalent, the second formula gives some
intuition about the terms that result from the original estimator. In the
numerator there is a factor due to the summation of independent random variables
and the dispersion this causes according to the Central Limit Theorem (CLT). In
the denominator the term represents the estimator normalisation.

When $f$ is smooth, complexity for a **Quasi Monte Carlo** (QMC) estimator of
function $f$ can equal:

$$
\mathcal{O}\left(N^{-1.5}\right)
\quad\text{or}\quad
\mathcal{O}\left(\frac{N^{-0.5}}{N}\right)
$$

More specifically, this is true for some Randomised Quasi Monte Carlo (RQMC)
estimators when $N\in\lbrace2^k|k\in\mathbb{N}\rbrace$. Relative to MC, the
efficiency factor is $N^3$. This is why QMC sampling becomes more critical with
higher sample counts. For low sample counts, other features like blue noise
dithering can have a greater impact on quality.

Practically we can't expect to always see such efficiency gains in real world
scenarios. These results are the best case, and typically what you see only when
the function is smooth. However, QMC is never worse than MC, and usually lands
somewhere between a notable improvement, and a dramatic improvement, as seen
here. This depends on the integral being estimated.

These plots show the rate of convergence for different implementations across
different two-dimensional integrals. Each plot takes the average of 128 runs,
plotting a data point for each sample count value.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/error-plot-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/error-plot-dark.png">
  <img alt="Error plot comparison." src="./images/plots/error-plot-light.png">
</picture>

The following plots show, for a fixed sample count, how the error decreases with
a Gaussian filter as the standard deviation increases. It shows, especially for
low sample counts, the faster convergence of the blue noise variants. This is
an indicator that these may be a better option if your images are then filtered
with a de-noise pass.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/error-filter-space-plot-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/error-filter-space-plot-dark.png">
  <img alt="Error filter space plot comparison." src="./images/plots/error-filter-space-plot-light.png">
</picture>

The preceding plots filtered the error spatially. The following plots filter
the error temporally. They show that averaging the frames over time also
favours the blue noise variants. Useful if you feed the frames into a temporal
antialiasing algorithm.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/error-filter-time-plot-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/error-filter-time-plot-dark.png">
  <img alt="Error filter time plot comparison." src="./images/plots/error-filter-time-plot-light.png">
</picture>

### Zoneplates

These plots show zoneplate tests for each sampler implementation, and give an
indication of any potential for structural aliasing across varying frequencies
and directions. Often this manifests as a moiré pattern.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/zoneplate-plot-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/zoneplate-plot-dark.png">
  <img alt="Zoneplate test comparison." src="./images/plots/zoneplate-plot-light.png">
</picture>

### Performance

In these charts you can see the time it takes to compute N samples for each
implementation. Each sample takes 256 dimensions. It also shows the cost of
cache initialisation, this is often a trade-off for runtime performance. The
plot measured the results on an Apple Silicon M1 Max with `OPENQMC_ARCH_TYPE`
set to `ARM` for Neon intrinsics.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/performance-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/performance-dark.png">
  <img alt="Performance comparison." src="./images/plots/performance-light.png">
</picture>

### Pixel sampling

These are path-traced images of the Cornell box comparing the results from each
implementation. The top row of insets shows pixel variance due to the visibility
term; the bottom row shows the variance of sampling a light at a grazing angle.
Each inset had 2 samples per pixel. Numbers indicate RMSE.

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

## Concepts and examples

OpenQMC aims to provide an API that makes using high quality samples easy and
bias-free when writing software with numerical integration. You have already
seen a very basic example of this in the [Usage](#usage) section. This section
will go into more detail on the concepts and provide some more complex examples.

The section will build upon a simple example, introduce the concepts of domain
trees and sample splitting, and show some of the potential pitfalls OpenQMC
prevents by following some basic rules. After this, you should be able to
confidently make use of QMC sampling in your own software.

### Sampling domains

When you estimate an integral using the Monte Carlo method, the high dimensional
space can often be partitioned into logical domains. In OpenQMC domains are an
important concept. Domains act as promises that can be converted into dimensions
upon request by the caller. They can also be used to derive other domains.

This example function tries to compute a pixel estimate. This estimate is made
up of two parts, each needing a domain. First is a `camera` that takes a single
dimension, and the second is a `light`, that takes two dimensions.

```cpp
Result estimatePixel(const oqmc::PmjBnSampler pixelDomain,
                     const CameraInterface& camera,
                     const LightInterface& light)
{
	enum DomainKey
	{
		Next,
	};

	// Derive 'cameraDomain' from 'pixelDomain' parameter.
	const auto cameraDomain = pixelDomain.newDomain(DomainKey::Next);

	// Derive 'lightDomain' from 'cameraDomain' variable.
	const auto lightDomain = cameraDomain.newDomain(DomainKey::Next);

	// Take a single dimension for time to pass to 'camera'.
	float timeSample[1];
	cameraDomain.drawSample<1>(timeSample);

	// Take two dimensions for direction to pass to 'light'.
	float directionSample[2];
	lightDomain.drawSample<2>(directionSample);

	// Pass drawn dimensions to the interface for importance sampling.
	const auto cameraSample = camera.sample(timeSample);
	const auto lightSample = light.sample(directionSample);

	return estimateLightTransport(cameraSample, lightSample);
}
```

The function started with an initial `pixelDomain`, from that it derived a
`cameraDomain`, and from that it derived a `lightDomain`. This is a linear
sequence of dependent domains. Such an operation is called **chaining**.

Domains provide up to 4 dimensions each, often referred to here as a sample
(technically part of a sample). If more than 4 dimensions are required, the
caller can always derive a new domain.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/diagrams/domain-tree-graph-1-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/diagrams/domain-tree-graph-1-dark.png">
  <img alt="lattice pair plot." src="./images/diagrams/domain-tree-graph-1-light.png">
</picture>

This example works, but isn't very flexible. This can be improved. Types that
implement the camera or light interfaces may require less or more dimensions
than is provided by the calling code.

### Passing domains

Computing domains is cheap, but drawing samples is expensive. In the
last example, a sample was drawn and passed to a type that implements the
`LightInterface`. If the sample is not required, the type doesn't have the
option to prevent the sample from being drawn.

You can resolve this by changing the interface to pass a domain, deferring the
decision to draw a sample to the type. Here are two types that implement such an
interface, `DiskLight` and `PointLight`.

```cpp
LightSample DiskLight::sample(const oqmc::PmjBnSampler lightDomain)
{
	// Draw two dimensions for direction.
	float directionSample[2];
	lightDomain.drawSample<2>(directionSample);

	// Compute LightSample object using the drawn sample...
}

LightSample PointLight::sample(const oqmc::PmjBnSampler lightDomain)
{
	// Don't draw two dimensions for direction.
	// float directionSample[2];
	// lightDomain.drawSample<2>(directionSample);

	// Compute LightSample object without using a drawn sample...
}
```

The `DiskLight` draws a sample from the domain. But `PointLight` opts to save on
compute and does not draw a sample as it is not required.

Looking at the revised calling code, the domains are now passed through the
interfaces, allowing the types to decide whether to draw a sample or not.

```cpp
Result estimatePixel(const oqmc::PmjBnSampler pixelDomain,
                     const CameraInterface& camera,
                     const LightInterface& light)
{
	enum DomainKey
	{
		Next,
	};

	// Derive 'cameraDomain' from 'pixelDomain' parameter.
	const auto cameraDomain = pixelDomain.newDomain(DomainKey::Next);

	// Derive 'lightDomain' from 'cameraDomain' variable.
	const auto lightDomain = cameraDomain.newDomain(DomainKey::Next);

	// Pass domains directly to the interface to compute a sample.
	const auto cameraSample = camera.sample(cameraDomain);
	const auto lightSample = light.sample(lightDomain);

	return estimateLightTransport(cameraSample, lightSample);
}
```

Sometimes a type needs an unknown number of domains. The next section will show
how that can be achieved. This will introduce a potential danger, as well as a
new concept called domain trees that can be used to do the operation safely.

### Domain branching

Building on the previous example, the calling code is free of knowing how many
domains a type might need. If a type requires more sample draws, it can always
derive a new domain from the domain that was passed to it, and then draw more
samples from those domains.

This is an example where `ThinLensCamera` implements the `CameraInterface`. It
requires multiple domains and uses chaining to sequentially derive domains one
after the other, starting with the original `cameraDomain`.

```cpp
CameraSample ThinLensCamera::sample(const oqmc::PmjBnSampler cameraDomain)
{
	enum DomainKey
	{
		Next,
	};

	// Derive 'lensDomain' from 'cameraDomain' parameter.
	const auto lensDomain = cameraDomain.newDomain(DomainKey::Next);

	// Derive 'timeDomain' from 'lensDomain' variable.
	const auto timeDomain = lensDomain.newDomain(DomainKey::Next);

	// Take two dimensions for lens to compute a CameraSample object.
	float lensSample[2];
	lensDomain.drawSample<2>(lensSample);

	// Take a single dimension for time to compute a CameraSample object.
	float timeSample[1];
	timeDomain.drawSample<1>(timeSample);

	// Compute CameraSample object using the drawn samples...
}
```

*Here is the dangerous part!* Notice how the calling `estimatePixel` function
derives both `lensDomain` and `lightDomain` from `cameraDomain`, meaning they
become equivalent in value.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/diagrams/domain-tree-graph-2-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/diagrams/domain-tree-graph-2-dark.png">
  <img alt="lattice pair plot." src="./images/diagrams/domain-tree-graph-2-light.png">
</picture>

This will result in lens and light dimensions correlating, leaving gaps in the
primary sample space. That type of correlation will bias your estimates, and
should be avoided. A revised version of the `estimatePixel` function:

```cpp
Result estimatePixel(const oqmc::PmjBnSampler pixelDomain,
                     const CameraInterface& camera,
                     const LightInterface& light)
{
	enum DomainKey
	{
		Camera,
		Light,
	};

	// Derive 'cameraDomain' from 'pixelDomain' parameter.
	const auto cameraDomain = pixelDomain.newDomain(DomainKey::Camera);

	// Derive 'lightDomain' from 'pixelDomain' parameter.
	const auto lightDomain = pixelDomain.newDomain(DomainKey::Light);

	// Pass domains directly to the interface to compute samples.
	const auto cameraSample = camera.sample(cameraDomain);
	const auto lightSample = light.sample(lightDomain);

	return estimateLightTransport(cameraSample, lightSample);
}
```

You may notice that the `DomainKey` enum has now changed. The new code derives
both `cameraDomain` and `lightDomain` directly from `pixelDomain`, but in each
instance passes a different enum value.

This operation is called **branching**. The derived domains are now branched and
independent. You can branch a domain on a given key either with an enum as seen
here, or by passing an integer value directly.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/diagrams/domain-tree-graph-3-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/diagrams/domain-tree-graph-3-dark.png">
  <img alt="lattice pair plot." src="./images/diagrams/domain-tree-graph-3-light.png">
</picture>

Now that `cameraDomain` and `lightDomain` are independent, the `ThinLensCamera`
can use the domain passed to it in whichever way it needs.

Mapping domains to the code's call graph creates domain trees. If you carefully
derive domains by using chaining when writing loops, and branching when passing
to interfaces, you'll avoid gaps in the primary sample space, guaranteeing
bias-free results.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/diagrams/domain-tree-graph-4-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/diagrams/domain-tree-graph-4-dark.png">
  <img alt="lattice pair plot." src="./images/diagrams/domain-tree-graph-4-light.png">
</picture>

OpenQMC's design makes domain trees a first-class concept and lets you focus on
writing complex and correct sampling code by applying the techniques described.

Here is what this example has identified. Gaps in the primary sample space
produce bias. Branching can be used to make domains independent of one another.
This independence prevents such bias. Finally, domain trees should match the
call graph of the code to guarantee bias-free results. For a more complete
example, see the [trace](src/tools/lib/trace.cpp) tool.

### Domain splitting

Whereas domain branching lets you sample different dimensions within each sample
index, domain **splitting** is an operation to sample given dimensions with a
higher sampling rate. Splitting allows you to tackle high variance in specific
dimensions without the cost for the increased sampling rate on all dimensions.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/diagrams/domain-tree-graph-8-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/diagrams/domain-tree-graph-8-dark.png">
  <img alt="lattice pair plot." src="./images/diagrams/domain-tree-graph-8-light.png">
</picture>

The diagram above shows the same domain tree extended with a stack for each
domain. Each stack element represents an index in the domain's pattern. This
example begins with two indices for each domain.

If you were to consider direct light sampling, which is often a source of high
variance. One possible approach is to sample lights at a higher rate than pixels
by defining a multiplier on that rate, such as `N_LIGHT_SAMPLES`.

```cpp
Result estimatePixel(const oqmc::PmjBnSampler pixelDomain,
                     const CameraInterface& camera,
                     const LightInterface& light)
{
	enum DomainKey
	{
		Camera,
		Light,
	};

	// Compute 'cameraSample' as in previous examples.
	const auto cameraDomain = pixelDomain.newDomain(DomainKey::Camera);
	const auto cameraSample = camera.sample(cameraDomain);

	auto result = Result{};

	// Loop a fixed N times.
	for(int i = 0; i < N_LIGHT_SAMPLES; ++i)
	{
		// Compute 'lightSample' using the newDomainSplit API.
		const auto lightDomain = pixelDomain.newDomainSplit(DomainKey::Light, N_LIGHT_SAMPLES, i);
		const auto lightSample = light.sample(lightDomain);

		// Average the pixel estimates from each light sample.
		result += estimateLightTransport(cameraSample, lightSample) / N_LIGHT_SAMPLES;
	}

	return result;
}
```

Here the calling code derives a `lightDomain` in a loop, using a different
function than the other examples. Along with a key, this new function takes a
fixed sample rate multiplier and an index within the multiplier range. You must
pass all indices to the API and average the results.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/diagrams/domain-tree-graph-5-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/diagrams/domain-tree-graph-5-dark.png">
  <img alt="lattice pair plot." src="./images/diagrams/domain-tree-graph-5-light.png">
</picture>

In this domain tree, `N_LIGHT_SAMPLES` maintains a fixed value of 2, and thus,
the light domain's sampling rate is twice that of the source pixel domain.

A fixed sample rate multiplier like the one in this example correlates points
globally *and* locally. This correlation makes the pattern optimal while saving
compute costs in other domains. The following section will give options for when
the sample rate multiplier is non-constant.

### Adaptive rate multipliers

Sometimes, the constraint of a fixed sample rate multiplier is not an option or
is undesirable. In these cases, a couple of alternative strategies trade quality
for the added flexibility of non-constant multipliers.

```cpp
Result estimatePixel(const oqmc::PmjBnSampler pixelDomain,
                     const CameraInterface& camera,
                     const LightInterface& light)
{
	enum DomainKey
	{
		Camera,
		Light,
	};

	// Compute 'cameraSample' as in previous examples.
	const auto cameraDomain = pixelDomain.newDomain(DomainKey::Camera);
	const auto cameraSample = camera.sample(cameraDomain);

	// Compute a non-fixed number of light samples.
	const auto nLightSamples = light.adaptiveSampleRate();

	auto result = Result{};

	// Loop a non-fixed N times.
	for(int i = 0; i < nLightSamples; ++i)
	{
		// Compute 'lightSample' using the newDomainDistrib API.
		const auto lightDomain = pixelDomain.newDomainDistrib(DomainKey::Light, i);
		const auto lightSample = light.sample(lightDomain);

		// Average the pixel estimates from each light sample.
		result += estimateLightTransport(cameraSample, lightSample) / nLightSamples;
	}

	return result;
}
```

This example has a non-constant sample rate multiplier `nLightSample` for the
light domain. Here, the caller uses the alternative `newDomainDistrib` function
that allows for varying sample rate multipliers. Using this technique is called
the **distribution strategy**.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/diagrams/domain-tree-graph-6-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/diagrams/domain-tree-graph-6-dark.png">
  <img alt="lattice pair plot." src="./images/diagrams/domain-tree-graph-6-light.png">
</picture>

Here is the domain tree when using the distribution strategy. The split domain
is not correlated globally, but it is locally. The domain is in fact branched
into two locally correlated domains, one for each index from the source pixel
domain. The first has a sample rate of 2, and the second a sample rate of 3,
based on `nLightSamples`.

```cpp
Result estimatePixel(const oqmc::PmjBnSampler pixelDomain,
                     const CameraInterface& camera,
                     const LightInterface& light)
{
	enum DomainKey
	{
		Camera,
		Light,
	};

	// Compute 'cameraSample' as in previous examples.
	const auto cameraDomain = pixelDomain.newDomain(DomainKey::Camera);
	const auto cameraSample = camera.sample(cameraDomain);

	// Compute a non-fixed number of light samples.
	const auto nLightSamples = light.adaptiveSampleRate();

	auto result = Result{};

	// Loop a non-fixed N times.
	for(int i = 0; i < nLightSamples; ++i)
	{
		// Compute 'lightSample' by chaining the newDomain API.
		const auto lightDomain = pixelDomain.newDomain(DomainKey::Light).newDomain(i);
		const auto lightSample = light.sample(lightDomain);

		// Average the pixel estimates from each light sample.
		result += estimateLightTransport(cameraSample, lightSample) / nLightSamples;
	}

	return result;
}
```

This last example is similar to the distribution strategy. But here the caller
uses the `newDomain` function, chaining and branching the domains on the index,
instead of splitting. This technique is called the **chaining strategy**.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/diagrams/domain-tree-graph-7-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/diagrams/domain-tree-graph-7-dark.png">
  <img alt="lattice pair plot." src="./images/diagrams/domain-tree-graph-7-light.png">
</picture>

Here is the domain tree when using the chaining strategy. This new domain is
correlated globally, but it is not locally. Each of the sample indices from the
source pixel domain is free to branch at non-constant rate. The first branches
into 2 domains, and the second into 3 domains, based on `nLightSamples`.

*So which strategy to choose?* Following are the results from each strategy.
Numbers after each strategy indicate RMSE. As expected, the initial option of
splitting using a fixed sample rate multiplier produces the best results.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/cornell-box-sample-splitting-2-8-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/cornell-box-sample-splitting-2-8-dark.png">
  <img alt="sample plitting 2 and 8." src="./images/plots/cornell-box-sample-splitting-2-8-light.png">
</picture>

But when that isn't possible and using a non-constant sample rate multiplier,
and that multiplier is expected to be higher than the source pixel sample rate,
the distribution strategy is optimal as it is locally correlated.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/cornell-box-sample-splitting-8-2-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/cornell-box-sample-splitting-8-2-dark.png">
  <img alt="sample plitting 8 and 2." src="./images/plots/cornell-box-sample-splitting-8-2-light.png">
</picture>

However, when an adaptive sample rate multiplier is expected to be lower than
the original pixel sample rate, as demonstrated here, the chaining strategy is
optimal as it is globally correlated.

## Implementation details

This section will go into detail about each back-end implementation, as well as
the blue noise variants. Here you can find out about practical trade-offs for
each option, so you can decide which is best for your use case.

### PMJ sequences

The implementation uses the stochastic method described by Helmer et la. [^2]
to efficiently construct a progressive multi-jittered (0,2) sequence. The first
pair of dimensions in a domain have the same integration properties as the
Sobol implementation. However, as the sequence doesn't extend to more than two
dimensions, the second pair is randomised relative to the first in a single
domain.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/diagrams/pmj-design-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/diagrams/pmj-design-dark.png">
  <img alt="PMJ pair plot." src="./images/diagrams/pmj-design-light.png">
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
  <img alt="PMJ pair plot." src="./images/plots/pair-plot-pmj-light.png">
</picture>

### Sobol sequences

The implementation uses an elegant construction by Burley [^1] for an Owen
scrambled Sobol sequence. This also includes performance improvements such as
limiting the index to 16 bits, pre-inverting the input and output matrices, and
making use of CPU vector intrinsics. You need to select a `OPENQMC_ARCH_TYPE` to
make use of the performance from vector intrinsics for a given architecture.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/diagrams/sobol-design-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/diagrams/sobol-design-dark.png">
  <img alt="sobol pair plot." src="./images/diagrams/sobol-design-light.png">
</picture>

This sampler has no cache initialisation cost, it generates all samples on
the fly without touching memory. However, the cost per draw sample call is
computationally higher than other samplers. The quality of Owen scramble
sequences often outweigh this cost due to their random error cancellation and
incredibly high rate of integration for smooth functions.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/pair-plot-sobol-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/pair-plot-sobol-dark.png">
  <img alt="Sobol pair plot." src="./images/plots/pair-plot-sobol-light.png">
</picture>

### Lattice sequences

The implementation uses the generator vector from Hickernell et al. [^3] to
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
low computation cost for a single draw sample call. However, the rate of
integration per pixel can be lower when compared to other samplers.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/plots/pair-plot-lattice-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/plots/pair-plot-lattice-dark.png">
  <img alt="Lattice pair plot." src="./images/plots/pair-plot-lattice-light.png">
</picture>

### Blue noise sampler variants

Blue noise variants offer spatial temporal blue noise dithering between pixels,
with progressive pixel sampling. This is done using an offline optimisation
process that's based on the work by Belcour and Heitz [^4], and extends
temporally as described by Wolfe et al. [^5].

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/diagrams/optimise-index-seed-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/diagrams/optimise-index-seed-dark.png">
  <img alt="lattice pair plot." src="./images/diagrams/optimise-index-seed-light.png">
</picture>

Each variant achieves a blue noise distribution using two pixel tables, one
holds keys to seed the sequence, and the other index ranks. These tables are
then randomised by toroidally shifting the table lookups for each domain using
random offsets. Correlation between the offsets and the pixels allows for a
single pair of tables to provide keys and ranks for all domains.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="./images/diagrams/blue-noise-procedure-light.png">
  <source media="(prefers-color-scheme: dark)" srcset="./images/diagrams/blue-noise-procedure-dark.png">
  <img alt="lattice pair plot." src="./images/diagrams/blue-noise-procedure-light.png">
</picture>

Although the spatial temporal blue noise doesn't reduce the error for an
individual pixel, it does give a better perceptual result due to less low
frequency structure in the error between pixels. Also, if an image is either
spatially or temporally filtered (as with denoising or temporal antialiasing),
the resulting error can be much lower when using a blue noise variant as shown
in [Rate of convergence](#rate-of-convergence).

Blue noise variants are recommended, as the additional performance cost will
likely be a favourable tradeoff for the quality gains at low sample counts.
However, access to the data tables can impact performance depending on the
architecture (e.g. GPU), so it is worth benchmarking if this is a concern.

For more details, see the Optimise CLI in the [Tools](#tools) section.

### Passing and packing samplers

Sampler objects can be efficiently passed by value into functions, as well as
packed and queued for deferred evaluation. Achieving this means that the memory
footprint of a sampler must be very small, and the type trivially copyable.

Sampler types are either 8 or 16 bytes in size depending on the type. The small
memory footprint is possible due to the state size of PCG-RXS-M-RX-32 from the
PCG family of PRNGs as described by O'Neill [^6].

When deriving domains the sampler will use an LCG state transition, and only
perform a permutation prior to drawing samples analogous to PCG. This provides
high quality bits when drawing samples, but keeps the cost low when deriving
domains, which might not be used.

## Development roadmap

Roadmap for version 1.0.0 of the library:

- Gather feedback and iterate on API.
- Add support for package managers.

## Developer workflow

Beyond the library itself there are a range of tools and tests that make up the
project as a whole. These are there to aid development and to provide an element
of analysis. These extra components also have more requirements and dependencies
than the library alone.

### Installing LFS

Some of the files in the repository require Git Large File Storage which is a
common Git extension for handling large file types. You can find installation
instructions on the [LFS](https://git-lfs.github.com) website.

### Dependencies

The tools and tests have dependencies on external projects. If these are already
installed on the system they can be found automatically. If a dependency isn't
installed, then it will be downloaded and compiled along with the project.

The dependencies are:

- [GLM](https://github.com/g-truc/glm)
- [GoogleTest](https://github.com/google/googletest)
- [Hypothesis](https://github.com/joshbainbridge/hypothesis)
- [oneTBB](https://github.com/oneapi-src/oneTBB)

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
- `OPENQMC_FORCE_DOWNLOAD`: Force dependencies to download and build, even if
  they are installed. Useful for guaranteeing compatibility. Option values can
  be `ON` or `OFF`. Default value is `OFF`.

### Build configuration

You can use configurations for [Extra build options](#extra-build-options)
based on your operating system by referencing pre-defined CMake presets. The
preset for Linux and macOS has the name `unix`. Presets initialise the build
config so that it's ready for development. Do this by running:

```bash
cmake --preset unix
```

You can also override the options using the CMake CLI while initialising the
build config. If the build already exists, then CMake updates the option:

```bash
cmake --preset unix -D OPENQMC_FORCE_DOWNLOAD=ON
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
for this in [src/tools/lib](src/tools/lib) folder. Here is a list of some of the
key tools:

- [`src/tools/lib/benchmark.cpp`](src/tools/lib/benchmark.cpp) : Measure sampler performance.
- [`src/tools/lib/generate.cpp`](src/tools/lib/generate.cpp): Generate sample value tables.
- [`src/tools/lib/trace.cpp`](src/tools/lib/trace.cpp): Render a path traced image.
- [`src/tools/lib/optimise.cpp`](src/tools/lib/optimise.cpp): Run a blue noise optimisation.

You can access the library's C API via a Python CTypes wrapper. On Unix this
just requires the `TOOLSPATH` environment variable to point towards the shared
library binary and importing the [python/wrapper.py](python/wrapper.py) module.

This library is also linked against multiple CLI commands found in the
[src/tools/cli](src/tools/cli) folder. Once you have compiled the tools, you
can run the CLI binaries from the command line as shown:

<details>
<summary>Benchmark CLI usage</summary>

```
The 'benchmark' tool measures the time for cache initialisation, as well as the
draw sample time, independently for each implementation. The results depend on
the hardware, as well as the build configuration.

USAGE: ./build/src/tools/cli/benchmark <sampler> <measurement>

ARGS:
  <sampler> Options are 'pmj', 'pmjbn', 'sobol', 'sobolbn', 'lattice', 'latticebn'.
  <measurement> Options are 'init', 'samples'.
```

</details>

<details>
<summary>Generate CLI usage</summary>

```
The 'generate' tool evaluates a specific implementation and outputs a table of
points from the sequence. Currently the CLI sets the table output options to 2
sequences, 256 samples, and 8 dimensions.

USAGE: ./build/src/tools/cli/generate <sampler>

ARGS:
  <sampler> Options are 'pmj', 'sobol', 'lattice'.
```

</details>

<details>
<summary>Trace CLI usage</summary>

```
The 'trace' tool is a CPU / GPU uni-directional path tracer that provides an
example of how you might use the API in a renderer. There are multiple scenes
embedded in the source code, including the Cornell box used on this page. This
also demonstrates how each sampler implementation practically performs.

USAGE: ./build/src/tools/cli/trace <sampler> <scene>

ARGS:
  <sampler> Options are 'pmj', 'pmjbn', 'sobol', 'sobolbn', 'lattice', 'latticebn'.
  <scene> Options are 'box', 'presence', 'blur'.
```

</details>

<details>
<summary>Optimise CLI usage</summary>

```
The 'optimise' tool targets a single base sampler implementation and produces
the data needed to construct a spatial temporal blue noise variant. The output
is two tables, files named 'keys.txt' and 'ranks.txt'.

The table data can be mapped to a 3D array, with the axes representing 2D pixel
coordinates and 1D time. The optimisation process works toroidally, so you can
tile each table to cover a large spatial and temporal area.

Keys are 32 bit integers, and seed unique sampler domains. Ranks are also 32 bit
integers, and when XORed with the sample index allow for spatial temporal blue
noise with progressive and adaptive rendering.

When optimising final quality results it's advisable to use a GPU build of the
optimiser due to the computational cost. Testing with an NVIDIA RTX A6000 found
that this provided a speedup of ~400x that of the CPU.

USAGE: ./build/src/tools/cli/optimise <sampler>

ARGS:
  <sampler> Options are 'pmj', 'sobol', 'lattice'.
```

</details>

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
CTest CLI from the project root directory. Run the command:

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

As a developer, you can set up the project using the `setup` recipe. This will
run the commands described in [Build configuration](#build-configuration) using
the `unix` preset:

```bash
just setup
```

By default, just asks CMake to use [Ninja](https://ninja-build.org) as the build
generator. Instructions on getting Ninja are on their site. If you would like to
use a different generator, you can pass a parameter to `setup`:

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

Images on this page are generated with a build of the tools library, and then
executing the notebooks from the command line. This is done twice, once for each
colour theme. The process is automated in the following script:

```bash
./scripts/save-notebook-images.sh
```

### Nix development environment

On NixOS or systems with Nix available, the provided flake can be used to load a
working and reproducible developer environment. Load this with:

```bash
nix develop
```

For the extra dependencies and environment variables required to run the Jupyter
notebooks, there is a dedicated development environment:

```bash
nix develop .#notebook
```

## Related projects

Here are details on other related projects, and how OpenQMC is different. If
OpenQMC doesn't solve your problem, one of these might be a better option:

- [Uni{corn|form} toolkit](https://github.com/utk-team/utk) is a great resource,
  but is more intended for research rather than production use.
- [Andrew Helmer](https://github.com/Andrew-Helmer/stochastic-generation) gives
  a great example implementation of their sampler, but no general interface.
- [Pbrt](https://github.com/mmp/pbrt-v4) is a seminal resource on all things
  rendering, but is written for educational purposes.

## Contributing

All contributions to the project are welcome. For guidance on how to contribute
please see the following pages:

- [CONTRIBUTING.md](CONTRIBUTING.md), process and procedures for contributing
  work towards the project. This includes sign-off instructions, coding style,
  as well as other requirements for a pull request.
- [GOVERNANCE.md](GOVERNANCE.md), overview of the governing procedure, and
  the difference between project maintainer roles. The process of becoming a
  maintainer is outlined here.
- [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md), covers the code of conduct that we
  are committed to upholding as participants of the project's development, and
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
