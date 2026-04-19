# Option A: Compile Time Sampler Selection

Detail for Option A of the [C ABI specification](spec.md). This design exposes
the generic C API defined in the specification, with the sampler
implementation chosen once, when the library is compiled. One sampler exists
per build of the library.

Note that the illustrative code below needs correction before use; it has not
been compiled.

## Selection

The implementation is a single translation unit. A preprocessor definition
selects the sampler it wraps; with no definition it defaults to PmjBn, the
recommended sampler. Each branch is three lines: an include, an alias, and a
name.

```cpp
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
```

Because only the selected header is included, only that sampler's
implementation and data tables are compiled and linked. A build wrapping
Sobol carries no blue noise tables.

## Implementation

The marshalling between the opaque value and the C++ type is written once,
against the `Sampler` alias. Copies go through `memcpy`, which the compiler
removes for small trivially copyable types, and two static assertions make an
oversized or non-trivial sampler a compile error.

```cpp
#include <oqmc/oqmc_c.h>

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

The entry points are then direct, one line each:

```cpp
extern "C"
{

const char* oqmc_sampler_name(void)
{
	return samplerName;
}

size_t oqmc_sampler_cache_size(void)
{
	return Sampler::cacheSize;
}

void oqmc_sampler_initialise_cache(void* cache)
{
	Sampler::initialiseCache(cache);
}

oqmc_sampler oqmc_sampler_create(int x, int y, int frame, int index,
                                 const void* cache)
{
	return store(Sampler(x, y, frame, index, cache));
}

oqmc_sampler oqmc_sampler_new_domain(const oqmc_sampler* sampler, int key)
{
	return store(load(sampler).newDomain(key));
}

void oqmc_sampler_draw_sample_f(const oqmc_sampler* sampler, int size,
                                float* sample)
{
	draw(load(sampler), size, sample);
}

// Remaining functions follow the same pattern.

} // extern "C"
```

## Building

Under CMake, a cache variable selects the sampler and maps to the compile
definition:

```cmake
set(OPENQMC_CABI_SAMPLER "pmjbn" CACHE STRING
    "Sampler implementation wrapped by the C ABI library.")
```

The file also compiles without CMake, as a single command, so a consumer can
vendor `cabi.cpp` and the headers directly into their own build:

```sh
c++ -O2 -I include -DOQMC_CABI_SAMPLER_SOBOL -c src/cabi.cpp -o cabi.o
cc main.c cabi.o -o main
```

Once the `stochastic.h` allocation moves to `malloc` (see *Runtime
Dependencies* in the specification), the link step needs no C++ runtime.

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
- **Vendorable** — compiles with a single compiler command, no build system
  required.
- **The trade** — changing sampler requires recompiling one translation unit,
  and a prebuilt package bakes the choice in for all of its consumers.
