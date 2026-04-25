# Migrate project to C++17 standard and reduce complexity

GitHub Issue: https://github.com/AcademySoftwareFoundation/openqmc/issues/86

## Summary

Migrate OpenQMC from C++14 to C++17 to eliminate the binary/header-only
install split and reduce build system complexity. C++17 `inline` variables
remove the need for the `OQMC_ENABLE_BINARY` mechanism entirely, allowing the
library to remain header-only without duplicating large static arrays across
compilation units.

## Motivation

OpenQMC includes six large blue noise lookup tables (key and rank tables for
PMJ, Sobol, and Lattice samplers) defined in `include/oqmc/bntables.h`. Each
table is 2^16 entries of `uint32_t`, totalling roughly 1.5 MB of data across
all six.

In C++14, `constexpr` namespace-scope variables have internal linkage. When
`bntables.h` is included from multiple translation units, each unit gets its
own copy of the table data in the final binary. To avoid this, the project
offers a binary variant controlled by the `OPENQMC_ENABLE_BINARY` CMake
option. When enabled, the tables are declared `extern` in the header and
defined once in `src/bntables.cpp`, which is compiled into a static or shared
library.

This creates user-facing complexity:

- Three CMake options (`OPENQMC_ENABLE_BINARY`, `OPENQMC_SHARED_LIB`,
  `OPENQMC_FORCE_PIC`) that exist solely for this purpose.
- Conditional `INTERFACE` vs `STATIC`/`SHARED` library target logic in
  `src/CMakeLists.txt`.
- A preprocessor split (`#if defined(OQMC_ENABLE_BINARY)`) in `bntables.h`.
- Documentation explaining the trade-off between header-only and binary
  installs.

C++17 introduces `inline` variable declarations. An `inline constexpr`
namespace-scope variable has external linkage and is guaranteed to have a
single definition across all translation units. This directly solves the
duplication problem with no build system workaround required.

## Changes — Headers

Replace the conditional compilation in `include/oqmc/bntables.h`. The current
pattern for each sampler namespace (repeated for `pmj`, `sobol`, `lattice`):

```cpp
#if defined(OQMC_ENABLE_BINARY)
extern const std::uint32_t keyTable[size];
extern const std::uint32_t rankTable[size];
#else
constexpr std::uint32_t keyTable[] = {
#include "data/pmj/keys.txt"
};
constexpr std::uint32_t rankTable[] = {
#include "data/pmj/ranks.txt"
};
#endif
```

Becomes:

```cpp
inline constexpr std::uint32_t keyTable[] = {
#include "data/pmj/keys.txt"
};
inline constexpr std::uint32_t rankTable[] = {
#include "data/pmj/ranks.txt"
};
```

The `#if defined(OQMC_ENABLE_BINARY)` blocks and corresponding `#else` /
`#endif` are removed entirely.

## Changes — Source code

### `src/bntables.cpp`

Delete this file. It only exists to provide `extern` definitions for the
binary variant.

## Changes — Build system

### `CMakeLists.txt` (root)

Remove the following options:

```cmake
option(OPENQMC_ENABLE_BINARY "Build binary to reduce memory cost.")
option(OPENQMC_SHARED_LIB "Make a shared library, in place of static.")
option(OPENQMC_FORCE_PIC "Force PIC for static libraries.")
```

### `src/CMakeLists.txt`

Replace the conditional library creation:

```cmake
if(OPENQMC_ENABLE_BINARY)
	if(OPENQMC_SHARED_LIB)
		add_library(${PROJECT_NAME} SHARED)
	else()
		add_library(${PROJECT_NAME} STATIC)
	endif()

	if(OPENQMC_FORCE_PIC)
		set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
	endif()

	target_sources(${PROJECT_NAME} PRIVATE bntables.cpp)
	target_compile_options(${PROJECT_NAME} PRIVATE ${OPENQMC_CXX_FLAGS})
	target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include)
else()
	add_library(${PROJECT_NAME} INTERFACE)
endif()
```

With:

```cmake
add_library(${PROJECT_NAME} INTERFACE)
```

Remove the `OQMC_ENABLE_BINARY` compile definition block:

```cmake
if(OPENQMC_ENABLE_BINARY)
	target_compile_definitions(${PROJECT_NAME} INTERFACE OQMC_ENABLE_BINARY)
endif()
```

Change the C++ standard requirement:

```cmake
# Before
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_14)

# After
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)
```

### `CMakePresets.json`

The `unix` preset sets `CMAKE_CXX_STANDARD` to `"14"` (line 18). Update to
`"17"`.

### `cmake/examples/`

Update `cxx_std_14` to `cxx_std_17` in the manual include example.

## Changes — Documentation

### `README.md`

- Change "This C++14 (CPU, GPU) header only library" to reference C++17.
- Update the requirements section: change "no dependencies other than C++14"
  to C++17 and adjust VFX Reference Platform compatibility to CY2021.
- Remove or update references to the binary install variant and its CMake
  options throughout the document.

Update minimum tested compiler versions:

- **GCC**: GCC 7 is the minimum requirement for C++17. No change needed.
- **Clang**: Clang 5 is the minimum requirement for C++17. No change needed.
- **NVCC**: Minimum version moves from NVCC 10 to NVCC 11. Update tested
  compiler documentation and CI matrix accordingly.

### `mkdocs/home.html`

- Update "Header-only C++14 library" and "CY2018" references on line 77.

## Migration risks

- **Downstream breakage**: Projects using `OPENQMC_ENABLE_BINARY`,
  `OPENQMC_SHARED_LIB`, or `OPENQMC_FORCE_PIC` will see CMake errors on
  upgrade. This is an intentional breaking change. A CHANGELOG entry and
  migration note should accompany the release.
- **VFX Reference Platform**: This migration moves the minimum from CY2018
  to CY2021.
- **Per-TU parse cost**: `OPENQMC_ENABLE_BINARY` solved two problems at once.
  As well as avoiding C++14's internal-linkage duplication in the final
  binary, defining the tables in a single `src/bntables.cpp` meant only that
  one translation unit had to parse the ~393K lines of integer literals in
  `include/oqmc/data/`. Moving to header-only `inline constexpr` fixes the
  duplication but reintroduces the parse cost in *every* translation unit that
  includes `bntables.h`. This is the headline trade-off of the migration and
  needs to be measured before it is accepted (see Validation).

## Validation

The following must be carried out before the migration is accepted. They are
not yet done.

- **NVCC device-linkage**: The blue noise tables are consumed from
  `OQMC_HOST_DEVICE` code (`bntables.h` `tableValue`), so they must be usable
  on the device. The current internal-linkage `constexpr` arrays produce a
  per-TU copy, which is what makes them trivially available in device code
  today. `inline constexpr` changes this to a single external-linkage
  definition, and NVCC's handling of inline-variable device-side definitions
  has historically had caveats. Confirm on the minimum supported toolchain
  (NVCC 11) that the tables compile, link, and produce correct results in
  device code with no duplicate-definition or missing-symbol errors.

- **Compile-time trade-off**: A test/benchmark must be created to measure the
  per-TU parse cost described under Migration risks on a representative
  consumer (several translation units each including `bntables.h`). Compare
  the C++14 binary variant, C++14 header-only, and the proposed C++17
  `inline constexpr` builds for total compile time and final binary size, and
  record the results here so the trade-off can be accepted explicitly.

## Candidate C++17 improvements

Beyond the core `inline constexpr` change, the following are candidate
improvements enabled by C++17 that could be included in this migration or
follow-up work.

### Hex float literals

`include/oqmc/float.h` line 34:

```cpp
// Before
constexpr auto floatOneOverTwoPower32 = 1.0f / (1ull << 32); ///< 0x1p-32

// After
constexpr auto floatOneOverTwoPower32 = 0x1p-32f;
```

Hex float literals (P3.9) are a C++17 feature. This replaces a computed
constant with an exact representation, removing the reliance on compile-time
evaluation of the division.

### `[[maybe_unused]]` attribute

The project defines a custom `OQMC_MAYBE_UNUSED` macro in
`include/oqmc/unused.h` (line 11) to suppress unused variable warnings:

```cpp
#define OQMC_MAYBE_UNUSED(exp) (void)(exp)
```

This is used in 6 locations across the codebase:

- `include/oqmc/stochastic.h` line 36 — `maxIndexSize`
- `include/oqmc/lattice.h` lines 51, 62 — `cache` parameter
- `include/oqmc/sobol.h` lines 51, 62 — `cache` parameter
- `src/shapes.h` lines 95, 110 — `x` and `y` parameters

C++17 provides `[[maybe_unused]]` as a standard attribute. All uses can be
replaced, and `unused.h` can be deleted. For example:

```cpp
// Before
OQMC_MAYBE_UNUSED(cache);

// After (on parameter)
[[maybe_unused]] const void* cache
```

### Nested namespace declarations

`include/oqmc/bntables.h` uses nested namespaces with separate declarations:

```cpp
namespace oqmc
{
namespace bntables
{
namespace pmj
{
```

C++17 allows compact nested namespace syntax:

```cpp
namespace oqmc::bntables::pmj
{
```

However, care should be taken to remain consistent with the rest of the
codebase. If adopted, it should apply uniformly (e.g. `bntables.h` uses 3
levels of nesting, while most other files use 1-2). This is a low-priority
stylistic change.

### `if constexpr`

`include/oqmc/owen.h` line 51 uses a runtime `if` to check dimension 0:

```cpp
if(dimension == 0)
{
    return reverseBits16(index);
}
```

This could become `if constexpr` if the function were templated on
`dimension`. However, this would be an API change and the compiler likely
optimises this already. Low priority.
