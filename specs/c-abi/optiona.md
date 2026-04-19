# Option A: Compile Time Sampler Selection

Detail for Option A of the [C ABI specification](spec.md). This design exposes
the generic C API defined in the specification, with the sampler
implementation chosen once, when the implementation is compiled. One sampler
exists per build.

The implementation lives in `oqmc_c.h` itself, after the declarations, behind
an `OQMC_IMPLEMENTATION` guard. OpenQMC compiles it into `liboqmc` and
distributes that. An integrator who prefers to can compile it into their own
build instead; see *Opt-in self compilation* under *Building*.

Note that the illustrative code below needs correction before use; it has not
been compiled.

## Selection

The implementation is a single translation unit. A preprocessor definition
selects the sampler it wraps; with no definition it defaults to PmjBn, the
recommended sampler. Each branch is three lines: an include, an alias, and a
name. The chain sits inside the implementation region of the header:

```cpp
#ifdef OQMC_IMPLEMENTATION
#ifndef __cplusplus
#error "OQMC_IMPLEMENTATION must be compiled as C++17."
#endif

#if defined(OQMC_CABI_SAMPLER_PMJ)
	#include <oqmc/pmj.h>
using Sampler = oqmc::PmjSampler;
constexpr const char* samplerName = "pmj";
#elif defined(OQMC_CABI_SAMPLER_SOBOL)
	#include <oqmc/sobol.h>
using Sampler = oqmc::SobolSampler;
constexpr const char* samplerName = "sobol";
// Remaining samplers follow the same pattern.
#else
	#include <oqmc/pmjbn.h>
using Sampler = oqmc::PmjBnSampler;
constexpr const char* samplerName = "pmjbn";
#endif

// ... implementation, below ...

#endif // OQMC_IMPLEMENTATION
```

Because only the selected header is included, only that sampler's
implementation and data tables are compiled and linked. A build wrapping
Sobol carries no blue noise tables.

The include guard of `oqmc_c.h` covers the declarations only. The
implementation region is guarded by `OQMC_IMPLEMENTATION` alone, so the header
can be included for declarations in any number of files and for the
implementation in exactly one. Included without the define it is a pure C11
header.

## Implementation

The marshalling between the opaque value and the C++ type is written once,
against the `Sampler` alias. Copies go through `memcpy`, which the compiler
removes for small trivially copyable types, and two static assertions make an
oversized or non-trivial sampler a compile error.

```cpp
static_assert(sizeof(Sampler) <= sizeof(oqmc_sampler),
              "Sampler must fit within the value storage.");
static_assert(std::is_trivially_copyable<Sampler>::value,
              "Sampler must be trivially copyable.");

static Sampler load(const oqmc_sampler* sampler)
{
	Sampler out;
	std::memcpy(&out, sampler, sizeof(out));

	return out;
}

static oqmc_sampler store(const Sampler& sampler)
{
	oqmc_sampler out{};
	std::memcpy(&out, &sampler, sizeof(sampler));

	return out;
}

template <typename T>
static void draw(const Sampler& sampler, int size, T* out)
{
	switch(size)
	{
	case 1: sampler.template drawSample<1>(out); break;
	case 2: sampler.template drawSample<2>(out); break;
	case 3: sampler.template drawSample<3>(out); break;
	case 4: sampler.template drawSample<4>(out); break;
	default: assert(size >= 1 && size <= 4); break;
	}
}
```

The entry points are then direct, one line each. Definitions carry the same
`OQMC_API` linkage macro as their declarations (see *Linkage*):

```cpp
extern "C"
{

OQMC_API const char* oqmc_sampler_name(void)
{
	return samplerName;
}

OQMC_API size_t oqmc_sampler_cache_size(void)
{
	return Sampler::cacheSize;
}

OQMC_API void oqmc_sampler_initialise_cache(void* cache)
{
	Sampler::initialiseCache(cache);
}

OQMC_API oqmc_sampler oqmc_sampler_create(int x, int y, int frame, int index,
                                          const void* cache)
{
	return store(Sampler(x, y, frame, index, cache));
}

OQMC_API oqmc_sampler oqmc_sampler_new_domain(const oqmc_sampler* sampler,
                                              int key)
{
	return store(load(sampler).newDomain(key));
}

OQMC_API void oqmc_sampler_draw_sample_f(const oqmc_sampler* sampler, int size,
                                         float* sample)
{
	draw(load(sampler), size, sample);
}

// Remaining functions follow the same pattern.

} // extern "C"
```

## Linkage

Every declaration and definition carries `OQMC_API`, defined at the top of
the header:

```c
#ifndef OQMC_API
#  ifdef OQMC_STATIC
#    define OQMC_API static
#  else
#    define OQMC_API extern
#  endif
#endif
```

The default, `extern`, is what `liboqmc` needs: symbols are exported so
consumers can link against it. Defining `OQMC_STATIC` before including the
header gives every function internal linkage instead. That exists for the
opt-in case: if two plugins loaded into one host process each compile the
implementation with different samplers, both export `oqmc_sampler_create`,
and on Linux the dynamic linker binds every call in the process to whichever
loaded first, so the second plugin silently runs the first plugin's sampler.
With `OQMC_STATIC` the two copies are invisible to each other. Because the
default is wrapped in `#ifndef`, an integrator may also define `OQMC_API` to
something else, such as a DLL export attribute, without editing the header.

## Building

`src/oqmc_impl.cpp` is two lines:

```cpp
#define OQMC_IMPLEMENTATION
#include <oqmc/oqmc_c.h>
```

Under CMake, a cache variable selects the sampler and maps to the compile
definition on that translation unit:

```cmake
set(OPENQMC_CABI_SAMPLER "pmjbn" CACHE STRING
    "Sampler implementation wrapped by the C ABI library.")
```

This produces `liboqmc`, which OpenQMC distributes. Consumers link it with a
C toolchain. Once the `stochastic.h` allocation moves to `malloc` (see
*Runtime Dependencies* in the specification), the link step needs no C++
runtime.

### Opt-in self compilation

An integrator who would rather not depend on an installed `liboqmc` writes
the same two lines in their own project and compiles them as C++17, choosing
the sampler themselves:

```sh
c++ -std=c++17 -O2 -I include -DOQMC_CABI_SAMPLER_SOBOL -c oqmc_impl.cpp
cc main.c oqmc_impl.o -o main
```

Nothing from OpenQMC's build system is involved, the sampler is chosen per
binary, and the translation unit cross compiles with whatever builds the rest
of the project. A plugin taking this route should also define `OQMC_STATIC`,
per *Linkage*. The integrator's build needs a C++17 compiler for this one
file; consumers with no C++ toolchain link the distributed `liboqmc`.

## Contributor Workflow

Adding a new sampler requires the existing C++ header, plus one branch in the
selection chain:

```cpp
#elif defined(OQMC_CABI_SAMPLER_MY)
	#include <oqmc/my.h>
using Sampler = oqmc::MySampler;
constexpr const char* samplerName = "my";
```

The public header does not change.

## Usage Example

```c
#include <oqmc/oqmc_c.h>

#include <stdlib.h>

int main(void)
{
	void* cache = malloc(oqmc_sampler_cache_size());
	oqmc_sampler_initialise_cache(cache);

	const oqmc_sampler pixel = oqmc_sampler_create(x, y, frame, index, cache);
	const oqmc_sampler camera = oqmc_sampler_new_domain(&pixel, 0);

	float sample[2];
	oqmc_sampler_draw_sample_f(&camera, 2, sample);

	free(cache);
	return 0;
}
```

An application that depends on a particular sampler can verify the build it
linked against with `oqmc_sampler_name()`, since the choice is otherwise
invisible in the ABI.

## Testing

In addition to the common tests in the specification:

- CI builds the library once per sampler definition, so every branch of the
  selection chain keeps compiling, and runs the equivalence tests against
  each build.

## Key Properties

- **Simplest possible implementation** — one translation unit, no dispatch,
  no global state, no macros beyond the selection chain.
- **Direct calls** — no indirection anywhere.
- **Minimal binary** — only the selected sampler's code and data tables are
  linked.
- **Fixed public header** — spelled out declarations with Doxygen comments;
  adding a sampler never changes it.
- **Self compilable** — the implementation is in the header, so an integrator
  can compile it with a single compiler command, no build system required.
- **The trade** — changing sampler requires recompiling one translation unit,
  and a prebuilt package bakes the choice in for all of its consumers.
  Integrators who need a different choice compile the implementation
  themselves.
