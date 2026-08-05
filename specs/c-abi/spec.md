# OQMC Sampler C ABI Specification

## Goal

Provide a stable C ABI for the OpenQMC sampler library that:

1. Preserves header-only, fully-inlined C++ usage for HPC.
2. Exposes a clean C API for cross-language FFI.
3. Minimises contributor boilerplate.
4. Supports easy sampler selection in user code.

Two candidate designs are described. This document gives an overview of each
and compares them; the full detail lives in [optiona.md](optiona.md) and
[optionb.md](optionb.md).

## Prerequisites

The C++14 to C++17 migration and removal of the existing `OPENQMC_ENABLE_BINARY`
CMake option must be completed before this spec is implemented. The current
binary mode only moves large lookup tables to a `.cpp` file (see `bntables.h`
`#if defined(OQMC_ENABLE_BINARY)` guards). This spec introduces a new
`OPENQMC_ENABLE_C_ABI` CMake option for the C ABI library build.

## Design Constraints

Both designs share these constraints.

- **Samplers are small value types** (8 or 16 bytes), trivially copyable, and
  frequently copied in HPC hot paths. The C wrapper must store the sampler
  **by value** (no heap allocation) so copies remain cheap.
- **Each sampler has a static `cacheSize`** and `initialiseCache()`. The cache
  is allocated externally and passed into the constructor. This keeps the
  sampler struct tiny while allowing arbitrarily large working memory.
- **The C API wraps the real `SamplerInterface<Impl>`**, which has:
  - A parametrised constructor: `SamplerInterface(x, y, frame, index, cache)`
  - Domain derivation: `newDomain(key)`, `newDomainSplit(key, size, index)`,
    `newDomainDistrib(key, index)`, `newDomainChain(key, index)`
  - Templated draw functions: `drawSample<Size>(out)`, `drawRnd<Size>(out)`
    with overloads for `uint32_t`, ranged `uint32_t`, and `float` output.
  - Static members: `cacheSize`, `initialiseCache(cache)`
  - A default constructor for placeholder objects.
- **Header-only by default**, with an optional compiled library build for
  languages that cannot instantiate C++ templates.
- **C++17** minimum standard for the C++ library (post-migration).
- **C11** minimum standard for the public C header.
- **GPU support**: the existing library supports CUDA via `OQMC_HOST_DEVICE`.
  The C ABI is CPU-only and does not need GPU annotations.

## Existing Samplers

The following sampler types exist and must be wrapped:

| Header         | Impl Type       | C++ Alias          | Size (bytes) | Alignment |
|----------------|-----------------|--------------------|--------------|-----------|
| `pmj.h`        | `PmjImpl`       | `PmjSampler`       | 16           | 8         |
| `pmjbn.h`      | `PmjBnImpl`     | `PmjBnSampler`     | 16           | 8         |
| `sobol.h`      | `SobolImpl`     | `SobolSampler`     | 8            | 4         |
| `sobolbn.h`    | `SobolBnImpl`   | `SobolBnSampler`   | 16           | 8         |
| `lattice.h`    | `LatticeImpl`   | `LatticeSampler`   | 8            | 4         |
| `latticebn.h`  | `LatticeBnImpl` | `LatticeBnSampler` | 16           | 8         |

Samplers without a cache pointer (Sobol, Lattice) are 8 bytes. Samplers with a
cache pointer (Pmj, and all blue noise variants) are 16 bytes. Note that
alignment is never equal to size, so the two must be specified separately.

## Naming Conventions

These match the existing project conventions visible in the codebase:

| Domain             | Convention                  | Example                            |
|--------------------|-----------------------------|------------------------------------|
| C++ types          | `UpperCamelCase`            | `PmjBnSampler`                     |
| C++ member funcs   | `lowerCamelCase`            | `drawSample()`, `cacheSize`        |
| C++ namespace      | `oqmc`                      | `oqmc::PmjBnSampler`               |
| C structs/funcs    | `lower_snake_case` + `oqmc_` prefix | `oqmc_sampler`, `oqmc_sampler_create` |
| Macros             | `ALL_CAPS` + `OQMC_` prefix | `OQMC_DECLARE_SAMPLER_C_API`       |

## Option A: Macro Generated Per-Sampler Symbols

Full detail in [optiona.md](optiona.md).

Preprocessor macros generate a separate set of C symbols for each sampler. A
declaration macro in the public header produces an opaque struct and a family of
functions per sampler; a definition macro in the `.cpp` produces their bodies.
A third macro, `OQMC_CHOOSE_SAMPLER`, lets user code alias one sampler to a
generic `oqmc_sampler` name.

```c
OQMC_CHOOSE_SAMPLER(pmjbn_sampler)

oqmc_sampler s = oqmc_sampler_create(x, y, frame, index, cache);

float sample[2];
oqmc_sampler_draw_sample_f(&s, 2, sample);
```

The implementation is fixed at compile time. Each sampler keeps its exact size,
so the C struct is 8 or 16 bytes as the C++ type is.

## Option B: Runtime Dispatch with a Type Registry

Full detail in [optionb.md](optionb.md).

A single opaque sampler type and a single set of functions. The value carries a
tag identifying its implementation, and each call dispatches through a
`constexpr` table of function pointers generated by templates. The
implementation is named directly as an enumerator, or resolved once from a
string.

```c
const oqmc_sampler_type type = oqmc_sampler_type_find("pmjbn");

const oqmc_sampler s = oqmc_sampler_create(type, x, y, frame, index, cache);

float sample[2];
oqmc_sampler_draw_sample_f(&s, 2, sample);
```

The implementation can be chosen at runtime. Storage is a fixed 24 bytes for
every sampler: a 20 byte payload followed by a 4 byte tag, which is 8 bytes
above the largest current sampler with 4 bytes of headroom for future growth.
The storage is 8 byte aligned, which the size being a multiple of 8 makes free.

## Measurements

The dispatch cost of Option B was measured against Option A and against the
inlined C++ library. The workload is 4.19 million units, each unit being one
`create`, two `newDomain` calls, and two `drawSample<2>` calls, with all calls
crossing a real shared library boundary. Figures are the ratio to the inlined
C++ time, and the range is the spread over repeated passes.

| Design | Sobol (8 bytes, compute bound) | PmjBn (16 bytes, table lookups) |
|---|---|---|
| Inlined C++ (header-only) | 1.00x | 1.00x |
| Option A, monomorphic symbols | 1.82x to 1.93x | 2.01x to 2.04x |
| Option B, table pointer in value | 1.78x to 1.91x | 2.09x to 2.10x |
| Option B, tag and `switch` | 1.79x to 1.91x | 2.10x to 2.11x |
| Option B, tag and table lookup | 1.55x to 1.59x | 1.93x to 1.98x |
| Batched, 64 indices per call | 1.23x to 1.32x | 1.13x to 1.16x |

Three conclusions:

1. **Dispatch is not a meaningful cost.** Within a single binary, where the
   comparison is clean, the dispatch strategies differ by one to three per
   cent, which is smaller than the pass-to-pass spread. A call site in a render
   loop is monomorphic, so the indirect branch predicts correctly after the
   first iteration.
2. **The boundary is the cost**, and Option A pays it in full as well. The
   direct call that Option A buys with macros and six symbol families is worth
   approximately nothing. Unlike C++ users, C consumers have no header-only
   fallback, so this is the defining performance characteristic of the C path.
3. **Batching would recover most of the difference**, and offers an order of
   magnitude more leverage than the choice of dispatch. Batch entry points can
   be added later without breaking the ABI, under either design.

Caveats, which matter if these numbers are quoted elsewhere. They come from a
single machine using GCC at `-O2` on x86-64 Linux, with cross library calls
resolved through the PLT. Absolute timings on that host drifted noticeably
between sessions, so only same-pass comparisons are meaningful, and the last
two rows were measured in separate binaries from the first three, which gives
them an unearned advantage from reduced instruction cache pressure. The ranking
of the dispatch strategies is robust; the absolute penalties should be
re-measured on a quiet machine before being relied upon.

A static library build, or hidden visibility with direct binding, would reduce
the boundary cost. It would reduce it equally for both options.

## Comparison

| Concern | Option A | Option B |
|---|---|---|
| Sampler selection | Compile time only, via `OQMC_CHOOSE_SAMPLER` | Runtime, by name |
| Public header | Macro generated declarations | Plain declarations, no macros at all |
| Doxygen coverage | Not possible for generated functions | Same as other headers |
| Symbol families | Six | One |
| ABI critical sizes | Six exact sizes, no slack | One reserved size |
| Future sampler growth | Breaks the ABI | Absorbed by reserved space |
| Edit sites per new sampler | Three | Two |
| Value size | 8 or 16 bytes | 24 bytes |
| Struct return | In registers | Through a hidden pointer |
| Dispatch overhead | None | Within noise of Option A |
| FFI surface | Six types to bind | One type to bind |
| Third party samplers | Not supported | Not supported (closed registry) |

Option A's advantages are a smaller value type and a direct call. The direct
call measures as noise. The smaller value type is real, but a consumer already
paying roughly twice the inlined cost for the boundary is unlikely to notice 24
bytes against 16 in an array.

Option B's cost is the larger value and a closed set of samplers: because the
tag is an index into a fixed registry, a third party cannot register an
implementation at runtime. A table pointer in the value would allow it, at the
price of position independence. Since the contributor workflow assumes in-tree
samplers, a closed registry is the honest model.

## Runtime Dependencies

### C++ runtime

A C consumer should ideally be able to link the C ABI library with a C
toolchain alone. The library is close to this but does not currently achieve
it.

Compiling the whole public API of all six samplers, plus every header under
`include/oqmc`, and linking the result as a plain C shared library leaves two
unresolved symbols:

```
undefined symbol: _Znam   (operator new[])
undefined symbol: _ZdaPv  (operator delete[])
```

Everything else resolves against libc: `memcpy`, `__assert_fail` and
`__stack_chk_fail`. There are no other cases. A sweep of the headers for heap
allocation, standard library containers, exceptions, run time type
information, virtual functions and function local statics finds a single site:
`stochastic.h` allocates a scratch buffer with
`new std::uint32_t[nsamples][2]` and releases it with `delete[]`. It is reached
through `pmj.h` and `pmjbn.h`, so it affects PMJ cache initialisation only.

Three ways to resolve it:

1. **Link `libstdc++`**, as `g++` does by default. Works, but a C consumer
   transitively acquires a C++ runtime.
2. **Link `libsupc++`** instead, which resolves both symbols with no
   `libstdc++` dependency.
3. **Replace the allocation with `std::malloc` and `std::free`**, which are
   libc. The buffer holds `std::uint32_t`, which needs no construction, so no
   placement new is required. Allocation failure changes from a thrown
   exception to a null pointer, which suits the assertion based precondition
   style already used throughout the library.

Option 3 was verified: with that single change, the dependency list of a C
executable linked against the C ABI library reduces to `libc.so.6` alone, with
no `libstdc++` and no `libgcc_s`, and sampler output is unchanged.

This is recorded here rather than addressed, as it is outside the scope of this
specification.

### C runtime

Going further and dropping libc as well is largely achievable, but not for a
build that includes every sampler. Measuring the remaining external symbols
under release flags gives:

| Symbol | Origin | Removed by |
|---|---|---|
| `__assert_fail` | `assert` throughout the library | `-DNDEBUG` |
| `__stack_chk_fail` | compiler stack protector | `-fno-stack-protector` |
| `strcmp` | the C ABI's own name lookup | hand-rolled comparison, a few lines |
| `memcpy` | blue noise `initialiseCache` bulk table copies | hand-rolled loop, at a cost |
| `malloc` / `free` | the `stochastic.h` scratch buffer | nothing short of an algorithm change |

Two findings shape what is realistically possible.

The `memcpy` calls all come from the three blue noise variants copying their key
and rank tables (`pmjbn.h`, `sobolbn.h`, `latticebn.h`). Every copy in the C ABI
shim itself is a compile time constant size and inlines away. Replacing a bulk
table copy with a hand written loop would be slower for no benefit, and in any
case a compiler may synthesise calls to `memcpy` and `memset` for ordinary
struct assignments regardless, so forbidding the symbol outright is not
something a library can guarantee.

The allocation cannot be removed without changing the PMJ construction. The
cache offers no room to borrow: it is exactly the 1 MB output table, and the
scratch buffer needs a further 512 KB. The algorithm reads the scratch at
permuted indices while writing the output, so it cannot run in place.

Verified result: a build restricted to Sobol and Lattice, compiled with
`-DNDEBUG -fno-stack-protector -fno-exceptions -fno-rtti -DOQMC_FORCE_SCALAR
-ffreestanding` and the hand-rolled name comparison, has **zero external
symbol dependencies**.

Note that `-ffreestanding` fails on any build using the vector paths: `owen.h`
includes `emmintrin.h`, and GCC's own `mm_malloc.h` requires `malloc` and
`free`. So a freestanding build implies the scalar path as well.

None of this is required for the C ABI. It is recorded because it bounds what
could be promised to embedded or kernel-adjacent consumers later: full
independence is reachable for the non-PMJ samplers on the scalar path, and
otherwise the floor is `memcpy` plus an allocator.

## Recommendation

Option B, storing a tag in the value and dispatching through a table.

The measurements remove the main objection to it, which was the cost of
dynamic dispatch. What remains is a design that gives runtime sampler
selection, a public header that can be read and documented, one ABI surface
rather than six, reserved space for future samplers, a single error path, and
one type for language bindings to wrap. The price is 8 additional bytes per
sampler value and a closed set of implementations.

If a C consumer is later shown to need the last few per cent, monomorphic fast
path symbols can be added alongside without breaking the ABI. The reverse is
not true: the six exact sizes baked into Option A's public header cannot be
relaxed later.

## Planned Language Support

The C ABI is part of a broader strategy to expose OpenQMC to multiple languages:

- **C++** — header-only (existing library, unchanged).
- **C** — via the C ABI defined in this spec (public C header + compiled library).
- **Zig** — via the C ABI (Zig has first-class C interop via `@cImport`).
- **Rust** — via the C ABI (using `bindgen` on the C header + link).
- **Python** — via nanobind, binding directly to the C++ API (separate effort, not through the C ABI).
