# OQMC Sampler C ABI Specification

## Goal

Provide a stable C ABI for the OpenQMC sampler library that:

1. Preserves header-only, fully-inlined C++ usage for HPC.
2. Exposes a clean C API for cross-language FFI.
3. Minimises contributor boilerplate.
4. Supports easy sampler selection.

Two candidate designs are described. Both expose the **same generic C API**,
defined below; they differ only in when the sampler implementation is
selected. Option A selects when the library is built. Option B selects once at
startup. Full detail lives in [optiona.md](optiona.md) and
[optionb.md](optionb.md).

## Prerequisites

The C++14 to C++17 migration and the removal of the `OPENQMC_ENABLE_BINARY`
CMake option are complete. This spec introduces a new `OPENQMC_ENABLE_C_ABI`
CMake option for the C ABI library build.

## Design Constraints

Both designs share these constraints.

- **Samplers are small value types** (8 or 16 bytes), trivially copyable, and
  frequently copied in HPC hot paths. The C wrapper must store the sampler
  **by value** (no heap allocation) so copies remain cheap.
- **Each sampler has a static `cacheSize`** and `initialiseCache()`. The cache
  is allocated externally, owned by the caller, and passed into the
  constructor.
- **The C API wraps the real `SamplerInterface<Impl>`**: the parametrised
  constructor, the four domain derivation functions, and the draw functions
  templated on size with `uint32_t` and `float` outputs.
- **C++17** minimum standard for the implementation, **C11** for the public
  header.
- **CPU only**: the C ABI carries no GPU annotations.

## Existing Samplers

| Header         | Impl Type       | C++ Alias          | Size (bytes) | Alignment |
|----------------|-----------------|--------------------|--------------|-----------|
| `pmj.h`        | `PmjImpl`       | `PmjSampler`       | 16           | 8         |
| `pmjbn.h`      | `PmjBnImpl`     | `PmjBnSampler`     | 16           | 8         |
| `sobol.h`      | `SobolImpl`     | `SobolSampler`     | 8            | 4         |
| `sobolbn.h`    | `SobolBnImpl`   | `SobolBnSampler`   | 16           | 8         |
| `lattice.h`    | `LatticeImpl`   | `LatticeSampler`   | 8            | 4         |
| `latticebn.h`  | `LatticeBnImpl` | `LatticeBnSampler` | 16           | 8         |

Samplers without a cache pointer (Sobol, Lattice) are 8 bytes. Samplers with a
cache pointer (Pmj, and all blue noise variants) are 16 bytes.

## Naming Conventions

| Domain           | Convention                          | Example                               |
|------------------|-------------------------------------|---------------------------------------|
| C++ types        | `UpperCamelCase`                    | `PmjBnSampler`                        |
| C++ member funcs | `lowerCamelCase`                    | `drawSample()`, `cacheSize`           |
| C structs/funcs  | `lower_snake_case` + `oqmc_` prefix | `oqmc_sampler`, `oqmc_sampler_create` |
| Macros           | `ALL_CAPS` + `OQMC_` prefix         | `OQMC_CABI_SAMPLER_PMJBN`             |

## File Structure

Both options use the same files. The existing per-sampler headers remain
unchanged and remain the primary interface for C++ users.

| File                          | Purpose                                     |
|-------------------------------|---------------------------------------------|
| `include/oqmc/*.h` (existing) | C++ header-only library, unchanged          |
| `include/oqmc/oqmc_c.h` (new) | Pure C11 header, no macros                  |
| `src/cabi.cpp` (new)          | The C ABI implementation, one translation unit |

## The C API

One opaque value type and one set of symbols, identical under both options.

```c
/// Sampler value.
///
/// A trivially copyable value type. Copy it by assignment, pass it by
/// pointer, store it in arrays. There is no destroy function; a sampler owns
/// no resources.
typedef struct oqmc_sampler
{
	uint64_t storage[2];
} oqmc_sampler;
```

Storage is **16 bytes**, the size of the largest sampler. Declaring it as an
array of `uint64_t` gives 8 byte alignment without `<stdalign.h>`; the size is
a multiple of the alignment, so every element of an array is itself aligned,
and 16 divides a cache line. A 16 byte struct is returned in two registers
under the System V AMD64 and AArch64 conventions (Windows x64 returns any
struct over 8 bytes through a hidden pointer regardless).

There is deliberately no growth headroom. A sampler larger than 16 bytes is an
ABI break, but the library's own design doctrine (see *Passing and packing
samplers* in `sampler.h`) already commits to samplers of at most 16 bytes, and
a static assertion in the implementation makes an oversized sampler a compile
error. Reserving space would give up register return to hold bytes the C++
library has promised never to need.

### Functions

Samplers are taken by pointer and returned by value. Self assignment is safe,
so a caller may walk a chain with `s = oqmc_sampler_new_domain(&s, key)`. The
`size` parameter of the draw functions is the number of dimensions, valid
range 1 to 4, and the caller provides an output array of at least `size`
elements. The ranged integer overloads of the C++ API can be added later.

| Symbol | Signature |
|---|---|
| `oqmc_sampler_name` | `const char* oqmc_sampler_name(void)` |
| `oqmc_sampler_cache_size` | `size_t oqmc_sampler_cache_size(void)` |
| `oqmc_sampler_initialise_cache` | `void oqmc_sampler_initialise_cache(void* cache)` |
| `oqmc_sampler_create` | `oqmc_sampler oqmc_sampler_create(int x, int y, int frame, int index, const void* cache)` |
| `oqmc_sampler_new_domain` | `oqmc_sampler oqmc_sampler_new_domain(const oqmc_sampler* sampler, int key)` |
| `oqmc_sampler_new_domain_split` | `oqmc_sampler oqmc_sampler_new_domain_split(const oqmc_sampler* sampler, int key, int size, int index)` |
| `oqmc_sampler_new_domain_distrib` | `oqmc_sampler oqmc_sampler_new_domain_distrib(const oqmc_sampler* sampler, int key, int index)` |
| `oqmc_sampler_new_domain_chain` | `oqmc_sampler oqmc_sampler_new_domain_chain(const oqmc_sampler* sampler, int key, int index)` |
| `oqmc_sampler_draw_sample_u` | `void oqmc_sampler_draw_sample_u(const oqmc_sampler* sampler, int size, uint32_t* sample)` |
| `oqmc_sampler_draw_sample_f` | `void oqmc_sampler_draw_sample_f(const oqmc_sampler* sampler, int size, float* sample)` |
| `oqmc_sampler_draw_rnd_u` | `void oqmc_sampler_draw_rnd_u(const oqmc_sampler* sampler, int size, uint32_t* rnd)` |
| `oqmc_sampler_draw_rnd_f` | `void oqmc_sampler_draw_rnd_f(const oqmc_sampler* sampler, int size, float* rnd)` |

`oqmc_sampler_name` returns the name of the implementation behind the API,
which Option A fixes at build time and Option B at select time, so an
application can verify it is running the sampler it expects.

## Option A: Compile Time Selection

Full detail in [optiona.md](optiona.md).

The implementation is chosen by a preprocessor definition when `cabi.cpp` is
compiled. One sampler exists per build of the library; only its code and data
tables are linked; every call is direct.

```c
/* Sampler chosen when the library was built. */
oqmc_sampler s = oqmc_sampler_create(x, y, frame, index, cache);
```

Changing sampler means recompiling one translation unit with a different
definition. The file compiles with a single compiler command, so it can also
be vendored into a consumer's own build.

## Option B: Runtime Selection

Full detail in [optionb.md](optionb.md).

All samplers are compiled into the library. The application selects one at
startup, defaulting to PmjBn if no selection is made. Every call then
dispatches through a single global table pointer.

```c
/* Sampler chosen once at startup. */
oqmc_sampler_select(OQMC_SAMPLER_TYPE_SOBOL);

oqmc_sampler s = oqmc_sampler_create(x, y, frame, index, cache);
```

One prebuilt binary serves every consumer, and the sampler can come from a
configuration file without a rebuild.

## Measurements

The cost of the C boundary and of dispatch was measured against the inlined
C++ library. The workload is 4.19 million units, each unit being one `create`,
two `newDomain` calls, and two `drawSample<2>` calls, with all calls crossing
a real shared library boundary. Figures are the ratio to the inlined C++ time,
and the range is the spread over repeated passes.

| Design | Sobol (8 bytes, compute bound) | PmjBn (16 bytes, table lookups) |
|---|---|---|
| Inlined C++ (header-only) | 1.00x | 1.00x |
| Monomorphic symbols (Option A) | 1.82x to 1.93x | 2.01x to 2.04x |
| Batched, 64 indices per call | 1.23x to 1.32x | 1.13x to 1.16x |

Three dispatch strategies for Option B were also measured: a table pointer in
the value, a tag with a `switch`, and a tag with a table lookup. All differed
from the monomorphic build by less than the pass-to-pass spread, so their
rows are omitted.

Three conclusions:

1. **The boundary is the cost, dispatch is not.** A call site in a render loop
   is monomorphic, so the indirect branch predicts correctly after the first
   iteration. C consumers have no header-only fallback, so the roughly 2x
   boundary is the defining performance characteristic of the C path under
   either option.
2. **The options do not differ on performance.** The direct call Option A
   retains measures as noise against Option B's dispatch.
3. **Batching is the real leverage**, an order of magnitude more than any
   dispatch choice. Batch entry points can be added later without breaking the
   ABI under either option.

Caveats: figures come from a single machine using GCC at `-O2` on x86-64
Linux, with cross library calls resolved through the PLT. Absolute timings
drifted between sessions, so only same-pass comparisons are meaningful, and
the batched row was measured in a separate binary from the others, which
gives it an unearned instruction cache advantage. Re-measure before relying
on the absolute penalties. A static library build, or hidden
visibility with direct binding, would reduce the boundary cost equally for
both options.

## Comparison

| Concern | Option A | Option B |
|---|---|---|
| Sampler selection | When the library is built | At startup |
| Selection from configuration | Requires a rebuild | Yes |
| Public header | Generic API only | Generic API plus type enum and select |
| Value size | 16 bytes, register return | 16 bytes, register return |
| Dispatch | Direct calls | One global table pointer, within noise |
| Code linked | Selected sampler only | All samplers and their data tables |
| Global state | None | One pointer, written at startup |
| Samplers per process | One, fixed at build | One, chosen at startup |
| Prebuilt packaging | One package per sampler | One package |
| Edit sites per new sampler | One selection branch | One enum entry, one registry entry |

Both options present the same sampler API, so the choice does not affect
calling code, only how a deployment selects the implementation. Consumers that
build from source, or vendor `cabi.cpp` into their own build, are better
served by Option A: no global state, no ordering rules, and only the selected
sampler is linked. Deployments that ship one prebuilt binary to many
consumers, such as system packages and central studio installs, are better
served by Option B: one artifact, with the sampler chosen from configuration.
Under either option a process holds exactly one sampler implementation, which
matches the intended use.

## Suggested Roadmap

Option B is a strict superset of Option A's surface: the value layout and
every symbol are identical, and B adds only the type enumeration and select.
Option A can therefore ship first, with Option B added later as a
compile time feature, and the addition is API only, so already compiled
binaries are unaffected. Two things keep that door open: a non-default
Option A build replaced by an Option B build reverts to the pmjbn default
until it selects, which `oqmc_sampler_name` can check; and Option A's direct
calls and absence of global state must remain documented as properties of
the build, not guarantees of the API.

## Runtime Dependencies

A C consumer should be able to link the C ABI library with a C toolchain
alone. Linking it today leaves two unresolved symbols, `operator new[]` and
`operator delete[]` (`_Znam`, `_ZdaPv`), with a single source: `stochastic.h`
allocates a scratch buffer with `new std::uint32_t[nsamples][2]`, reached
through PMJ cache initialisation.

Replacing that allocation with `std::malloc` and `std::free` removes the last
C++ runtime dependency. The buffer needs no construction, and allocation
failure changing from a thrown exception to a null pointer suits the assertion
based precondition style used throughout the library. This was verified: with
that single change, a C executable linked against the C ABI library depends on
`libc.so.6` alone, with no `libstdc++` and no `libgcc_s`, and sampler output
is unchanged. The change should land alongside the C ABI so that C consumers
never acquire a transitive C++ runtime.

## Build Integration

```cmake
option(OPENQMC_ENABLE_C_ABI "Build the C ABI shared/static library.")
```

When enabled, `src/cabi.cpp` is compiled into a shared or static library,
selected by the `OPENQMC_SHARED_LIB` and `OPENQMC_FORCE_PIC` options that were
removed with the C++17 migration and need to be reinstated. The `oqmc_c.h`
header installs with the other public headers regardless of the option. The
compiled target is exported as a separate CMake target that does not propagate
`cxx_std_17`, so a C only downstream project can consume it without a C++
toolchain requirement.

## Testing

Two test targets are needed under either option, because they check different
things:

- **A C compiled target** — a small program built by the C compiler with
  `-std=c11 -Wall -Wextra -pedantic`, linked against the C ABI library and
  nothing else. This is the only test that proves the header is valid C and
  that the symbols link from C.
- **`src/tests/cabi.cpp`** — a GoogleTest target that includes both
  `oqmc/oqmc_c.h` and the C++ headers, verifying the C results bit for bit
  against the equivalent `SamplerInterface` calls: `create`, all four domain
  derivations, and all draw functions at every size in 1 to 4, plus value
  semantics and self assignment.

Each option document adds the tests specific to its selection mechanism.

## Planned Language Support

The C ABI is part of a broader strategy to expose OpenQMC to multiple
languages:

- **C++** — header-only (existing library, unchanged).
- **C** — via the C ABI defined in this spec.
- **Zig** — via the C ABI (first-class C interop via `@cImport`).
- **Rust** — via the C ABI (`bindgen` on the C header, plus link).
- **Python** — via nanobind, binding directly to the C++ API (separate effort,
  not through the C ABI).
