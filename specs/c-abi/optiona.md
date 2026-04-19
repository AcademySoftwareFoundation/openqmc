# Option A: Macro Generated Per-Sampler Symbols

Detail for Option A of the [C ABI specification](spec.md). This design
generates a separate set of C symbols for each sampler using preprocessor
macros, with the implementation fixed at compile time in the calling code.

Note that the illustrative code below needs correction before use; it has not
been compiled.

## File Structure

The C ABI introduces new files alongside the existing headers. The existing
per-sampler `.h` headers (e.g. `pmj.h`, `sobolbn.h`) remain unchanged.

| File                        | Purpose                                   | Consumers              |
|-----------------------------|-------------------------------------------|------------------------|
| `include/oqmc/*.h` (existing) | C++ header-only: templates, implementations | C++ users              |
| `include/oqmc/cabi.h` (new)   | Pure C header: declarations + macros     | C users + other languages via FFI |
| `src/cabi.cpp` (new)          | C API definitions: linkable symbols      | Compiled library build |

### Existing C++ Headers (unchanged)

The existing headers under `include/oqmc/` already contain the `SamplerInterface`
template, concrete `*Impl` classes, and `using` aliases. These remain the primary
interface for C++ users. No changes are required.

### `include/oqmc/cabi.h` — Pure C Header (Declarations + Macros)

A pure C11 header with no C++ includes. Contains three macros and the per-sampler
declarations. This file does **not** include `sampler.h` or any other C++ header.

```c
// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#pragma once

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

// --- Macro: declare C API symbols for a sampler ---
#define OQMC_DECLARE_SAMPLER_C_API(c_name, byte_size)                          \
    typedef struct oqmc_##c_name                                               \
    {                                                                          \
        alignas(byte_size) char _opaque[byte_size];                            \
    } oqmc_##c_name;                                                           \
    size_t oqmc_##c_name##_cache_size(void);                                   \
    void oqmc_##c_name##_initialise_cache(void* cache);                        \
    oqmc_##c_name oqmc_##c_name##_create(int x, int y, int frame, int index,   \
                                         const void* cache);                   \
    oqmc_##c_name oqmc_##c_name##_new_domain(const oqmc_##c_name* s, int key); \
    oqmc_##c_name oqmc_##c_name##_new_domain_split(                            \
        const oqmc_##c_name* s, int key, int size, int index);                 \
    oqmc_##c_name oqmc_##c_name##_new_domain_distrib(                          \
        const oqmc_##c_name* s, int key, int index);                           \
    oqmc_##c_name oqmc_##c_name##_new_domain_chain(                            \
        const oqmc_##c_name* s, int key, int index);                           \
    void oqmc_##c_name##_draw_sample_u(const oqmc_##c_name* s, int size,       \
                                       uint32_t* sample);                      \
    void oqmc_##c_name##_draw_sample_f(const oqmc_##c_name* s, int size,       \
                                       float* sample);                         \
    void oqmc_##c_name##_draw_rnd_u(const oqmc_##c_name* s, int size,          \
                                    uint32_t* rnd);                            \
    void oqmc_##c_name##_draw_rnd_f(const oqmc_##c_name* s, int size,          \
                                    float* rnd);

// --- Macro: define C API function bodies (used in .cpp) ---
//
// The opaque C struct and the C++ type must have matching size and
// alignment. Static assertions verify this at compile time. Functions
// use memcpy to convert between the opaque byte array and the C++
// type, which the compiler will optimise away for small
// trivially-copyable types.

#define OQMC_DEFINE_SAMPLER_C_API(CPP_ALIAS, c_name)                           \
    static_assert(sizeof(oqmc_##c_name) == sizeof(oqmc::CPP_ALIAS),            \
                  "C struct size must match C++ type size");                    \
    static_assert(alignof(oqmc_##c_name) >= alignof(oqmc::CPP_ALIAS),          \
                  "C struct alignment must be sufficient for C++ type");        \
    static oqmc::CPP_ALIAS& toImpl(oqmc_##c_name& c)                          \
    {                                                                          \
        return reinterpret_cast<oqmc::CPP_ALIAS&>(c);                          \
    }                                                                          \
    static const oqmc::CPP_ALIAS& toImpl(const oqmc_##c_name& c)              \
    {                                                                          \
        return reinterpret_cast<const oqmc::CPP_ALIAS&>(c);                    \
    }                                                                          \
    static oqmc_##c_name fromImpl(const oqmc::CPP_ALIAS& impl)                \
    {                                                                          \
        oqmc_##c_name c;                                                       \
        std::memcpy(&c, &impl, sizeof(c));                                     \
        return c;                                                              \
    }                                                                          \
    size_t oqmc_##c_name##_cache_size(void)                                    \
    {                                                                          \
        return oqmc::CPP_ALIAS::cacheSize;                                     \
    }                                                                          \
    void oqmc_##c_name##_initialise_cache(void* cache)                         \
    {                                                                          \
        oqmc::CPP_ALIAS::initialiseCache(cache);                               \
    }                                                                          \
    oqmc_##c_name oqmc_##c_name##_create(int x, int y, int frame, int index,   \
                                         const void* cache)                    \
    {                                                                          \
        return fromImpl(oqmc::CPP_ALIAS(x, y, frame, index, cache));           \
    }                                                                          \
    oqmc_##c_name oqmc_##c_name##_new_domain(const oqmc_##c_name* s, int key)  \
    {                                                                          \
        return fromImpl(toImpl(*s).newDomain(key));                            \
    }                                                                          \
    oqmc_##c_name oqmc_##c_name##_new_domain_split(                            \
        const oqmc_##c_name* s, int key, int size, int index)                  \
    {                                                                          \
        return fromImpl(toImpl(*s).newDomainSplit(key, size, index));           \
    }                                                                          \
    oqmc_##c_name oqmc_##c_name##_new_domain_distrib(                          \
        const oqmc_##c_name* s, int key, int index)                            \
    {                                                                          \
        return fromImpl(toImpl(*s).newDomainDistrib(key, index));              \
    }                                                                          \
    oqmc_##c_name oqmc_##c_name##_new_domain_chain(                            \
        const oqmc_##c_name* s, int key, int index)                            \
    {                                                                          \
        return fromImpl(toImpl(*s).newDomainChain(key, index));                \
    }                                                                          \
    void oqmc_##c_name##_draw_sample_u(const oqmc_##c_name* s, int size,       \
                                       uint32_t* sample)                       \
    {                                                                          \
        switch(size)                                                           \
        {                                                                      \
        case 1: toImpl(*s).drawSample<1>(sample); break;                       \
        case 2: toImpl(*s).drawSample<2>(sample); break;                       \
        case 3: toImpl(*s).drawSample<3>(sample); break;                       \
        case 4: toImpl(*s).drawSample<4>(sample); break;                       \
        default: assert(!"size must be within [1, 4]"); break;                 \
        }                                                                      \
    }                                                                          \
    void oqmc_##c_name##_draw_sample_f(const oqmc_##c_name* s, int size,       \
                                       float* sample)                          \
    {                                                                          \
        switch(size)                                                           \
        {                                                                      \
        case 1: toImpl(*s).drawSample<1>(sample); break;                       \
        case 2: toImpl(*s).drawSample<2>(sample); break;                       \
        case 3: toImpl(*s).drawSample<3>(sample); break;                       \
        case 4: toImpl(*s).drawSample<4>(sample); break;                       \
        default: assert(!"size must be within [1, 4]"); break;                 \
        }                                                                      \
    }                                                                          \
    void oqmc_##c_name##_draw_rnd_u(const oqmc_##c_name* s, int size,          \
                                    uint32_t* rnd)                             \
    {                                                                          \
        switch(size)                                                           \
        {                                                                      \
        case 1: toImpl(*s).drawRnd<1>(rnd); break;                             \
        case 2: toImpl(*s).drawRnd<2>(rnd); break;                             \
        case 3: toImpl(*s).drawRnd<3>(rnd); break;                             \
        case 4: toImpl(*s).drawRnd<4>(rnd); break;                             \
        default: assert(!"size must be within [1, 4]"); break;                 \
        }                                                                      \
    }                                                                          \
    void oqmc_##c_name##_draw_rnd_f(const oqmc_##c_name* s, int size,          \
                                    float* rnd)                                \
    {                                                                          \
        switch(size)                                                           \
        {                                                                      \
        case 1: toImpl(*s).drawRnd<1>(rnd); break;                             \
        case 2: toImpl(*s).drawRnd<2>(rnd); break;                             \
        case 3: toImpl(*s).drawRnd<3>(rnd); break;                             \
        case 4: toImpl(*s).drawRnd<4>(rnd); break;                             \
        default: assert(!"size must be within [1, 4]"); break;                 \
        }                                                                      \
    }

// --- Macro: choose a sampler in user code ---
#define OQMC_CHOOSE_SAMPLER(c_name)                                            \
    typedef oqmc_##c_name oqmc_sampler;                                        \
    static inline size_t oqmc_sampler_cache_size(void)                         \
    {                                                                          \
        return oqmc_##c_name##_cache_size();                                   \
    }                                                                          \
    static inline void oqmc_sampler_initialise_cache(void* cache)              \
    {                                                                          \
        oqmc_##c_name##_initialise_cache(cache);                               \
    }                                                                          \
    static inline oqmc_sampler oqmc_sampler_create(                            \
        int x, int y, int frame, int index, const void* cache)                 \
    {                                                                          \
        return oqmc_##c_name##_create(x, y, frame, index, cache);              \
    }                                                                          \
    static inline oqmc_sampler oqmc_sampler_new_domain(                        \
        const oqmc_sampler* s, int key)                                        \
    {                                                                          \
        return oqmc_##c_name##_new_domain(s, key);                             \
    }                                                                          \
    static inline void oqmc_sampler_draw_sample_u(                             \
        const oqmc_sampler* s, int size, uint32_t* sample)                     \
    {                                                                          \
        oqmc_##c_name##_draw_sample_u(s, size, sample);                        \
    }                                                                          \
    static inline void oqmc_sampler_draw_sample_f(                             \
        const oqmc_sampler* s, int size, float* sample)                        \
    {                                                                          \
        oqmc_##c_name##_draw_sample_f(s, size, sample);                        \
    }                                                                          \
    static inline void oqmc_sampler_draw_rnd_u(                                \
        const oqmc_sampler* s, int size, uint32_t* rnd)                        \
    {                                                                          \
        oqmc_##c_name##_draw_rnd_u(s, size, rnd);                              \
    }                                                                          \
    static inline void oqmc_sampler_draw_rnd_f(                                \
        const oqmc_sampler* s, int size, float* rnd)                           \
    {                                                                          \
        oqmc_##c_name##_draw_rnd_f(s, size, rnd);                              \
    }                                                                          \
    /* Extend with additional domain forwarding functions as needed. */

// --- Sampler declarations ---
OQMC_DECLARE_SAMPLER_C_API(pmj_sampler, 16)
OQMC_DECLARE_SAMPLER_C_API(pmjbn_sampler, 16)
OQMC_DECLARE_SAMPLER_C_API(sobol_sampler, 8)
OQMC_DECLARE_SAMPLER_C_API(sobolbn_sampler, 16)
OQMC_DECLARE_SAMPLER_C_API(lattice_sampler, 8)
OQMC_DECLARE_SAMPLER_C_API(latticebn_sampler, 16)

#if defined(__cplusplus)
}
#endif
```

**How `OQMC_CHOOSE_SAMPLER` works**: The macro creates a `typedef` aliasing
the chosen sampler struct to a generic `oqmc_sampler` type, then generates
`static inline` forwarding functions that map generic names
(e.g. `oqmc_sampler_create`) to the concrete sampler's functions
(e.g. `oqmc_pmjbn_sampler_create`). This is the C equivalent of
`using Sampler = oqmc::PmjBnSampler;` in C++ — user code writes
`oqmc_sampler_*` everywhere, and switching samplers requires changing only
the single `OQMC_CHOOSE_SAMPLER` invocation.

The macro above forwards cache, create, draw, and `newDomain` functions.
The remaining domain derivation functions (`new_domain_split`,
`new_domain_distrib`, `new_domain_chain`) should also be forwarded in the
full implementation.

### `src/cabi.cpp` — C API Definitions

Includes the C++ headers and the C header, then expands `OQMC_DEFINE_SAMPLER_C_API`
for each sampler to produce real, externally-linkable symbols.

```cpp
// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include <oqmc/oqmc.h>
#include <oqmc/cabi.h>

extern "C"
{

OQMC_DEFINE_SAMPLER_C_API(PmjSampler, pmj_sampler)
OQMC_DEFINE_SAMPLER_C_API(PmjBnSampler, pmjbn_sampler)
OQMC_DEFINE_SAMPLER_C_API(SobolSampler, sobol_sampler)
OQMC_DEFINE_SAMPLER_C_API(SobolBnSampler, sobolbn_sampler)
OQMC_DEFINE_SAMPLER_C_API(LatticeSampler, lattice_sampler)
OQMC_DEFINE_SAMPLER_C_API(LatticeBnSampler, latticebn_sampler)

} // extern "C"
```

## CMake Integration

A new CMake option `OPENQMC_ENABLE_C_ABI` controls the C ABI library build.
This is independent of the (now removed) `OPENQMC_ENABLE_BINARY` option.

```cmake
option(OPENQMC_ENABLE_C_ABI "Build the C ABI shared/static library.")
```

When enabled, `src/cabi.cpp` is compiled into either a shared or static library.
That is configured by the `OPENQMC_SHARED_LIB` and `OPENQMC_FORCE_PIC` options
that was removed with the C++17 migration, and so will need to be brought back.
The `cabi.h` header is always installed as part of the public headers regardless
of this option.

Also when enabled, we should have a seperate C target that is installed and does
not require the C++17 dependency for downstream projects.

## Per-Sampler C API Surface

For each sampler registered as `c_name`, the following symbols are generated.
This maps the full `SamplerInterface` API to C:

### Cache management

| Symbol | Signature |
|---|---|
| `oqmc_<c_name>_cache_size` | `size_t oqmc_<c_name>_cache_size(void)` |
| `oqmc_<c_name>_initialise_cache` | `void oqmc_<c_name>_initialise_cache(void* cache)` |

### Construction

| Symbol | Signature |
|---|---|
| `oqmc_<c_name>_create` | `oqmc_<c_name> oqmc_<c_name>_create(int x, int y, int frame, int index, const void* cache)` |

### Domain derivation

| Symbol | Signature |
|---|---|
| `oqmc_<c_name>_new_domain` | `oqmc_<c_name> oqmc_<c_name>_new_domain(const oqmc_<c_name>* s, int key)` |
| `oqmc_<c_name>_new_domain_split` | `oqmc_<c_name> oqmc_<c_name>_new_domain_split(const oqmc_<c_name>* s, int key, int size, int index)` |
| `oqmc_<c_name>_new_domain_distrib` | `oqmc_<c_name> oqmc_<c_name>_new_domain_distrib(const oqmc_<c_name>* s, int key, int index)` |
| `oqmc_<c_name>_new_domain_chain` | `oqmc_<c_name> oqmc_<c_name>_new_domain_chain(const oqmc_<c_name>* s, int key, int index)` |

### Sample and RNG drawing

The `size` parameter specifies the number of dimensions to draw (valid range:
1–4). The caller must provide an output array of at least `size` elements.

| Symbol | Signature |
|---|---|
| `oqmc_<c_name>_draw_sample_u` | `void oqmc_<c_name>_draw_sample_u(const oqmc_<c_name>* s, int size, uint32_t* sample)` |
| `oqmc_<c_name>_draw_sample_f` | `void oqmc_<c_name>_draw_sample_f(const oqmc_<c_name>* s, int size, float* sample)` |
| `oqmc_<c_name>_draw_rnd_u` | `void oqmc_<c_name>_draw_rnd_u(const oqmc_<c_name>* s, int size, uint32_t* rnd)` |
| `oqmc_<c_name>_draw_rnd_f` | `void oqmc_<c_name>_draw_rnd_f(const oqmc_<c_name>* s, int size, float* rnd)` |

**Note**: The ranged-integer overloads (`drawSample(range, sample)` and
`drawRnd(range, rnd)`) can be added later if needed, or implemented by users
on top of the `uint32_t` output.

## Usage Examples

### C++

Unchanged from the existing library:

```cpp
#include <oqmc/pmjbn.h>

using Sampler = oqmc::PmjBnSampler;

auto cache = new char[Sampler::cacheSize];
Sampler::initialiseCache(cache);

const auto sampler = Sampler(x, y, frame, index, cache);

float sample[2];
sampler.drawSample<2>(sample);

const auto child = sampler.newDomain(key);

delete[] cache;
```

### C

```c
#include <oqmc/cabi.h>
#include <stdlib.h>

OQMC_CHOOSE_SAMPLER(pmjbn_sampler)

int main(void)
{
    size_t n = oqmc_sampler_cache_size();
    void* cache = malloc(n);
    oqmc_sampler_initialise_cache(cache);

    oqmc_sampler s = oqmc_sampler_create(x, y, frame, index, cache);

    float sample[2];
    oqmc_sampler_draw_sample_f(&s, 2, sample);

    oqmc_sampler child = oqmc_sampler_new_domain(&s, key);

    free(cache);
    return 0;
}
```

## Contributor Workflow

Adding a new sampler requires edits in three places:

1. **`include/oqmc/<sampler>.h`** — C++ implementation and alias (existing pattern):
   ```cpp
   class MyImpl { /* ... */ };
   using MySampler = SamplerInterface<MyImpl>;
   ```

2. **`include/oqmc/cabi.h`** — C API declaration (size must match `sizeof(MySampler)`):
   ```c
   OQMC_DECLARE_SAMPLER_C_API(my_sampler, 16)
   ```

3. **`src/cabi.cpp`** — C API definition:
   ```cpp
   OQMC_DEFINE_SAMPLER_C_API(MySampler, my_sampler)
   ```

Users can then select the new sampler in C code with:
```c
OQMC_CHOOSE_SAMPLER(my_sampler)
```

## Testing

A new test file `src/tests/cabi.cpp` validates the C ABI. Despite the `.cpp`
extension (required for GoogleTest), the tests should only use the C API
surface — `#include <oqmc/cabi.h>` and the `oqmc_*` functions — to verify
the ABI works as a C consumer would use it.

Tests should cover:

- **Size and alignment** — verify that each opaque struct has the expected
  `sizeof` and `alignof` values.
- **Cache lifecycle** — `_cache_size` returns a non-zero value (for samplers
  with caches), and `_initialise_cache` + `_create` produce a valid sampler.
- **Round-trip correctness** — for each sampler, verify that the C API
  `_draw_sample_f` output matches the equivalent C++ `drawSample<N>` call
  with the same construction parameters.
- **Domain derivation** — `_new_domain`, `_new_domain_split`,
  `_new_domain_distrib`, and `_new_domain_chain` produce samplers that
  draw different values from the parent.
- **Value semantics** — copying an `oqmc_<c_name>` struct via assignment
  produces an independent sampler that draws the same values as the original.
- **Invalid size** — `_draw_sample_*` and `_draw_rnd_*` with `size` outside
  [1, 4] triggers the assert (debug builds only).

## Key Properties

- **Header-only for C++** — maximum inlining, HPC-friendly, trivially copyable samplers.
- **Linkable C API** — stable `extern "C"` ABI for cross-language FFI.
- **Full API coverage** — wraps the complete `SamplerInterface` public API including domain derivation, all draw functions, and cache management.
- **Minimal boilerplate** — macros generate all repetitive C declarations and definitions.
- **Sampler selection macro** — `OQMC_CHOOSE_SAMPLER` lets C users swap samplers by changing one line.
- **No namespace pollution** — all C symbols prefixed with `oqmc_`, all macros prefixed with `OQMC_`.
- **Value semantics** — C struct is a fixed-size opaque byte array matching the C++ type's size. Samplers are passed and returned by value (no heap allocation), preserving cheap copies for HPC hot paths. A `static_assert` in the `.cpp` verifies size agreement at compile time.
- **No `destroy` function** — samplers are trivially copyable value types with no resources to release. There is no destructor.
- **Existing conventions preserved** — file headers use SPDX license identifiers, the `OQMC_CABI` pattern from `src/tools/lib/abi.h` is a precedent, and naming follows the established project style.
