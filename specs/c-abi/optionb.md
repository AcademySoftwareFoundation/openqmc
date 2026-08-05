# Option B: Runtime Dispatch with a Type Registry

Detail for Option B of the [C ABI specification](spec.md). This design exposes
a single sampler type and a single set of C functions. The implementation is
selected at runtime by name, and each call dispatches through a table of
function pointers held in read-only data.

## File Structure

| File                            | Purpose                                     | Consumers                         |
|---------------------------------|---------------------------------------------|-----------------------------------|
| `include/oqmc/*.h` (existing)   | C++ header-only: templates, implementations | C++ users                         |
| `include/oqmc/oqmc_c.h` (new)   | Pure C header: declarations only, no macros | C users + other languages via FFI |
| `src/cabi.cpp` (new)            | Thunks, type registry, C entry points       | Compiled library build            |

The header is named `oqmc_c.h` rather than `cabi.h`, on the basis that a public
include path should describe what a consumer wants rather than an internal
implementation concept. This is a permanent public name, so it is worth
settling deliberately.

## The Sampler Value

The C sampler is an opaque fixed size value that is **self describing**: it
carries both the sampler state and the identity of its implementation. That
identity is stored as a small integer tag, not as a pointer to a dispatch
table, because a tag is **position independent**. The value can be copied,
written to disk, queued for deferred evaluation, or moved between processes and
remain valid. A table pointer is an address within the loaded image, which
differs between runs under address space layout randomisation and is
meaningless in any other process. Since the small sampler footprint exists
precisely to support packing and queueing (see the *Passing and packing
samplers* section of `sampler.h`), storing a pointer would forfeit the property
the design is protecting.

Storage is **24 bytes**: a 20 byte payload followed by a 4 byte tag. The
largest current sampler is 16 bytes, so this is 8 bytes above the C++ type,
with 4 bytes of payload headroom for a future implementation to grow into. The
payload is placed first so that it inherits the alignment of the storage.

```c
/// Sampler value.
///
/// A trivially copyable value type holding both the sampler state and the
/// identity of its implementation. Copy it by assignment, pass it by pointer,
/// store it in arrays. There is no destroy function; a sampler owns no
/// resources. The uint64_t member gives the storage 8 byte alignment without
/// depending on <stdalign.h>.
typedef struct oqmc_sampler
{
	uint64_t storage[3];
} oqmc_sampler;
```

The size is written directly rather than through a named constant. Three
`uint64_t` is 24 bytes, and there is no second place that needs to refer to the
figure: the internal layout asserts against `sizeof(oqmc_sampler)` rather than
against a macro. This leaves the public header with **no macros at all**, only
the include guard and the `extern "C"` guard.

The size is nonetheless part of the ABI, and changing it breaks already compiled
downstream binaries.

The storage is **8 byte aligned**, which is free because the size is already a
multiple of 8. Declaring it as an array of `uint64_t` obtains that alignment
without `<stdalign.h>`, which is not a standard C++ header even though the same
file is compiled as C++.

Alignment is not needed for correctness, since every access copies through a
suitably aligned local object and `memcpy` imposes no alignment requirement on
either operand. It is specified anyway because it costs nothing and buys three
things:

- **Uniform alignment across arrays.** Size being a multiple of the alignment
  is what makes alignment propagate: with 24 bytes at 8 byte alignment, every
  element of an `oqmc_sampler` array is itself 8 byte aligned. At 20 bytes with
  4 byte alignment it would not be.
- **Wide moves on strict alignment targets.** On x86-64 and AArch64 unaligned
  access is full speed and the generated copy is the same either way. On a
  target that faults on unaligned access, `memcpy` of an unaligned object can
  degrade to a byte loop.
- **Room to drop the copy later.** An 8 byte aligned payload can be accessed in
  place by a future implementation. An unaligned one cannot, and raising the
  alignment of a published type is a breaking change, so this has to be decided
  now.

Note that 24 does not divide a 64 byte cache line, so some elements of a large
array will straddle lines whichever alignment is chosen. Only padding to 32
would avoid that, which is the cost the smaller value accepts.

Smaller layouts are possible, with trade-offs:

| Size | Layout | Trade-off |
|---|---|---|
| 24 | 20 byte payload, 4 byte tag | 4 bytes of headroom, 8 byte alignment for free |
| 20 | 16 byte payload, 4 byte tag | No headroom, and only 4 byte alignment |
| 17 | 16 byte payload, 1 byte tag | No headroom, and an awkward size for arrays |
| 16 | No tag in the value | Returns in registers, but the type must be passed to every call |

The 16 byte case is the only way to avoid growing the value at all, and it
means the caller must thread the type alongside the sampler through every
function. A mismatched pair is then undefined behaviour that the API cannot
detect, which is a poor trade for 8 bytes.

The value could in principle stay at 16 bytes by stealing the low bits of the
cache pointer, which is always 8 byte aligned, and using the spare half of the
cache-less samplers. This is rejected: it requires knowledge of each `Impl`
layout, which `SamplerInterface` deliberately keeps private, and it would break
silently the first time a sampler's internals changed.

## Type Identification

A sampler implementation is identified by an enumerator. The enumerator is also
the tag stored inside the value and the index into the dispatch registry, so
there is one concept rather than three.

```c
/// Sampler implementation.
///
/// Pass one of these to oqmc_sampler_create, either named directly or
/// resolved from a string by oqmc_sampler_type_find.
typedef enum oqmc_sampler_type
{
	OQMC_SAMPLER_TYPE_INVALID = -1, ///< Not a sampler; returned on lookup failure.
	OQMC_SAMPLER_TYPE_PMJ,          ///< Low discrepancy pmj sequence.
	OQMC_SAMPLER_TYPE_PMJBN,        ///< Blue noise pmj variant.
	OQMC_SAMPLER_TYPE_SOBOL,        ///< Owen scrambled Sobol sequence.
	OQMC_SAMPLER_TYPE_SOBOLBN,      ///< Blue noise Sobol variant.
	OQMC_SAMPLER_TYPE_LATTICE,      ///< Rank one lattice sequence.
	OQMC_SAMPLER_TYPE_LATTICEBN,    ///< Blue noise lattice variant.
} oqmc_sampler_type;

/// Resolve a sampler implementation from its name.
///
/// @param [in] name One of "pmj", "pmjbn", "sobol", "sobolbn", "lattice"
/// or "latticebn".
/// @return Matching implementation, or OQMC_SAMPLER_TYPE_INVALID if the name
/// is not recognised.
oqmc_sampler_type oqmc_sampler_type_find(const char* name);
```

An enumeration rather than an integer typedef gives four things. It keeps the
public header **free of macros**, which is one of this design's stated aims. It
makes the set of samplers **discoverable** in the header, where an opaque `int`
told the reader nothing. It gives FFI generators a **real type** to bind rather
than an alias for `int`. And it allows a consumer that already knows which
sampler it wants to **name it directly**, skipping the string lookup:

```c
const oqmc_sampler_type type = OQMC_SAMPLER_TYPE_PMJBN;
```

`oqmc_sampler_type_find` remains for name driven selection, from a command line
argument or a configuration file. It places the **entire error path in one
function**: the name is validated once, and every subsequent call takes a
resolved enumerator. No other function in the API needs to report an unknown
sampler, and no per-call string comparison is performed. Recognised names match
the CLI tools.

Two consequences worth stating. The enumerator values are the registry indices,
so the registry order in `cabi.cpp` must match the enumeration and **reordering
either is an ABI break**; new samplers are appended. And because one enumerator
is negative, the underlying type is signed, which is what makes the invalid
sentinel expressible; measured as 4 bytes under both C and C++.

## C API Surface

One set of symbols covers all implementations. Samplers are taken by pointer
and returned by value, matching Option A and the C++ interface.

Returning by value costs nothing here. A 24 byte struct exceeds the two
eightbyte limit for register returns under the System V AMD64 convention, the
8 byte limit under Windows x64, and the 16 byte limit under AArch64 AAPCS, so
it is returned through a hidden pointer supplied by the caller. That is
mechanically identical to an explicit out parameter, as confirmed by
inspecting the generated code. What by-value buys is a more natural API: the
result can be declared `const` at its point of definition and cannot be left
uninitialised.

Note the threshold this crosses. A 16 byte value would be returned in two
registers instead, which is the one concrete advantage Option A's smaller
struct retains, and the reason the 16 byte layout appears in the table above.

Draw functions take the sampler by pointer, so a hot loop never copies the
value in order to read from it.

### Type queries

| Symbol | Signature |
|---|---|
| `oqmc_sampler_type_find` | `oqmc_sampler_type oqmc_sampler_type_find(const char* name)` |
| `oqmc_sampler_type_cache_size` | `size_t oqmc_sampler_type_cache_size(oqmc_sampler_type type)` |
| `oqmc_sampler_type_initialise_cache` | `void oqmc_sampler_type_initialise_cache(oqmc_sampler_type type, void* cache)` |

Cache management hangs off the type rather than off a sampler, because the
cache must be initialised before any sampler exists.

### Construction and domain derivation

| Symbol | Signature |
|---|---|
| `oqmc_sampler_create` | `oqmc_sampler oqmc_sampler_create(oqmc_sampler_type type, int x, int y, int frame, int index, const void* cache)` |
| `oqmc_sampler_new_domain` | `oqmc_sampler oqmc_sampler_new_domain(const oqmc_sampler* sampler, int key)` |
| `oqmc_sampler_new_domain_split` | `oqmc_sampler oqmc_sampler_new_domain_split(const oqmc_sampler* sampler, int key, int size, int index)` |
| `oqmc_sampler_new_domain_distrib` | `oqmc_sampler oqmc_sampler_new_domain_distrib(const oqmc_sampler* sampler, int key, int index)` |
| `oqmc_sampler_new_domain_chain` | `oqmc_sampler oqmc_sampler_new_domain_chain(const oqmc_sampler* sampler, int key, int index)` |

Self assignment is safe, so a caller may reuse a variable while walking a chain
of domains with `s = oqmc_sampler_new_domain(&s, key)`. The implementation
copies the input into a local object before producing the result.

### Sample and RNG drawing

The `size` parameter is the number of dimensions to draw, valid range 1 to 4.
The caller provides an output array of at least `size` elements.

| Symbol | Signature |
|---|---|
| `oqmc_sampler_draw_sample_u` | `void oqmc_sampler_draw_sample_u(const oqmc_sampler* sampler, int size, uint32_t* sample)` |
| `oqmc_sampler_draw_sample_f` | `void oqmc_sampler_draw_sample_f(const oqmc_sampler* sampler, int size, float* sample)` |
| `oqmc_sampler_draw_rnd_u` | `void oqmc_sampler_draw_rnd_u(const oqmc_sampler* sampler, int size, uint32_t* rnd)` |
| `oqmc_sampler_draw_rnd_f` | `void oqmc_sampler_draw_rnd_f(const oqmc_sampler* sampler, int size, float* rnd)` |

As with Option A, the ranged integer overloads can be added later.

## Implementation

Templates generate the per-sampler code, replacing the macro expansion in
Option A. A `Vtable` holds one slot per public API function, `Thunks`
translates between the opaque payload and a concrete C++ sampler, and a
`constexpr` registry maps identifiers to tables.

The layout inside the reserved storage is private to the implementation:

```cpp
// Internal layout of the bytes held by oqmc_sampler. The tag identifies the
// implementation; the payload holds the C++ sampler object. The payload sits
// first so that it inherits the alignment of the storage. std::byte states
// that the payload is opaque storage rather than character or numeric data.
struct Value
{
	alignas(8) std::byte payload[20];
	std::uint32_t tag;
};

static_assert(sizeof(Value) == sizeof(oqmc_sampler),
              "Value must fill the reserved storage exactly.");
static_assert(alignof(oqmc_sampler) >= alignof(Value),
              "Storage must be at least as aligned as the payload.");
```

Thunks copy in and out with `memcpy`, which is what makes output aliasing safe
and avoids reading the payload through an incompatible type. Two static
assertions per sampler replace the hand written size literals of Option A, so a
sampler that outgrows the payload is a compile error rather than an ABI break:

```cpp
template <typename Sampler> struct Thunks
{
	static_assert(sizeof(Sampler) <= sizeof(Value::payload),
	              "Sampler must fit within the reserved payload.");
	static_assert(std::is_trivially_copyable<Sampler>::value,
	              "Sampler must be trivially copyable.");

	static Sampler load(const void* payload)
	{
		Sampler sampler;
		std::memcpy(&sampler, payload, sizeof(sampler));

		return sampler;
	}

	static void store(void* payload, const Sampler& sampler)
	{
		std::memcpy(payload, &sampler, sizeof(sampler));
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

	static void newDomain(const void* self, int key, void* out)
	{
		store(out, load(self).newDomain(key));
	}

	static void drawSampleF(const void* self, int size, float* sample)
	{
		draw(load(self), size, sample);
	}

	// Remaining members follow the same pattern.
};
```

A single `draw` template serves both the `float` and the `uint32_t` outputs
through the existing C++ overload set, so the size dispatch is written once
rather than four times.

The registry is a compile time constant, so the tables live in read-only data
with no runtime initialisation and no static initialisation order concerns:

```cpp
template <typename Sampler> constexpr Vtable makeVtable(const char* name)
{
	return Vtable{name,
	              Sampler::cacheSize,
	              &Thunks<Sampler>::initialiseCache,
	              &Thunks<Sampler>::create,
	              &Thunks<Sampler>::newDomain,
	              // Remaining slots follow the same pattern.
	              &Thunks<Sampler>::drawSampleF};
}

constexpr Vtable registry[] = {
    makeVtable<oqmc::PmjSampler>("pmj"),
    makeVtable<oqmc::PmjBnSampler>("pmjbn"),
    makeVtable<oqmc::SobolSampler>("sobol"),
    makeVtable<oqmc::SobolBnSampler>("sobolbn"),
    makeVtable<oqmc::LatticeSampler>("lattice"),
    makeVtable<oqmc::LatticeBnSampler>("latticebn"),
};
```

The array index is the public enumerator value, and the name sits beside it, so
the two cannot drift apart. Every entry point then follows one shape — read,
propagate the tag, index the registry, write:

```cpp
oqmc_sampler oqmc_sampler_new_domain(const oqmc_sampler* sampler, int key)
{
	const auto in = read(sampler);
	assert(valid(int(in.tag)));

	auto value = Value{};
	value.tag = in.tag;

	registry[in.tag].newDomain(in.payload, key, value.payload);

	return fromValue(value);
}
```

A table lookup is used in preference to a `switch` over sampler types. Both
dispatch equally well, but a `switch` would have to be repeated in every one of
the API functions, which reintroduces exactly the repetition that the macros in
Option A exist to remove.

## Contributor Workflow

Adding a new sampler requires edits in two places, one of which already exists
in the current pattern:

1. **`include/oqmc/<sampler>.h`** — C++ implementation and alias (existing
   pattern, unchanged):
   ```cpp
   class MyImpl { /* ... */ };
   using MySampler = SamplerInterface<MyImpl>;
   ```

2. **`src/cabi.cpp`** — one registry entry, appended:
   ```cpp
   makeVtable<oqmc::MySampler>("my"),
   ```

A matching enumerator is appended to `oqmc_sampler_type` so the sampler can be
named directly, though a consumer selecting by string needs no header change and
no recompilation. There is no size literal to maintain.

## Usage Example

The C++ usage is unchanged. In C:

```c
#include <oqmc/oqmc_c.h>

#include <stdio.h>
#include <stdlib.h>

enum DomainKey
{
	Camera,
	Light,
};

int main(int argc, char** argv)
{
	/* Sampler chosen at runtime, from a command line argument. */
	const char* name = argc > 1 ? argv[1] : "pmjbn";

	const oqmc_sampler_type type = oqmc_sampler_type_find(name);

	if(type == OQMC_SAMPLER_TYPE_INVALID)
	{
		fprintf(stderr, "unknown sampler '%s'\n", name);
		return 1;
	}

	void* cache = malloc(oqmc_sampler_type_cache_size(type));
	oqmc_sampler_type_initialise_cache(type, cache);

	const oqmc_sampler pixel =
	    oqmc_sampler_create(type, x, y, frame, index, cache);

	/* Branch two independent domains from the pixel domain. */
	const oqmc_sampler camera = oqmc_sampler_new_domain(&pixel, Camera);
	const oqmc_sampler light = oqmc_sampler_new_domain(&pixel, Light);

	float lens[2];
	oqmc_sampler_draw_sample_f(&camera, 2, lens);

	/* Split the light domain to sample it at 4x the pixel rate. */
	for(int i = 0; i < 4; ++i)
	{
		const oqmc_sampler split =
		    oqmc_sampler_new_domain_split(&light, Light, 4, i);

		float direction[2];
		oqmc_sampler_draw_sample_f(&split, 2, direction);
	}

	free(cache);
	return 0;
}
```

## CMake Integration

As with Option A, a new option controls the build:

```cmake
option(OPENQMC_ENABLE_C_ABI "Build the C ABI shared/static library.")
```

The `OPENQMC_SHARED_LIB` and `OPENQMC_FORCE_PIC` options removed during the
C++17 migration need to be reinstated to select between a shared and a static
build. The `oqmc_c.h` header installs with the other public headers regardless
of the option, as `include/CMakeLists.txt` already installs the whole `oqmc`
directory.

The compiled target must be exported as a separate CMake target that does not
propagate `cxx_std_17`, so a C only downstream project can consume it without a
C++ toolchain requirement. This needs its own export set and handling in
`cmake/Config.cmake.in`.

## Testing

Two test targets are needed, because they check different things:

- **`src/tests/cabi.cpp`** — a GoogleTest target that includes both
  `oqmc/oqmc_c.h` and the C++ headers, so it can verify the C results against
  the C++ results directly. This is the round-trip correctness check.
- **A C compiled target** — a small program built by the C compiler with
  `-std=c11 -Wall -Wextra -pedantic`, linked against the C ABI library and
  nothing else. This is the only test that proves the header is valid C and
  that the symbols link from C. A round trip through a C++ compiler cannot
  establish either.

Coverage should include:

- **Storage and alignment** — every sampler fits the reserved payload, and
  `sizeof` and `alignof` of `oqmc_sampler` are 24 and 8.
- **Type lookup** — every documented name resolves to the matching enumerator;
  an unknown name and a null pointer both return `OQMC_SAMPLER_TYPE_INVALID`.
- **Cache lifecycle** — `cache_size` is non-zero for the samplers that use a
  cache and zero for Sobol and Lattice, and a sampler constructed after
  `initialise_cache` draws valid values.
- **Equivalence with C++** — for every sampler, the C results match the
  equivalent `SamplerInterface` calls bit for bit, across `create`, all four
  domain derivations, and all draw functions at every size in 1 to 4.
- **Domain derivation** — derived domains draw different values from the parent
  and from each other, matching the guarantees in `sampler.h`.
- **Value semantics** — a copy made by assignment draws the same values as the
  original, and `s = oqmc_sampler_new_domain(&s, key)` gives the same result as
  assigning to a distinct variable.
- **Tag propagation** — a domain derived from a sampler reports the same type as
  its parent.
- **Invalid size** — a draw with `size` outside [1, 4] triggers the assert
  (debug builds only).

## Key Properties

- **One ABI surface** — a single value type and one set of symbols, so there is one reserved size to maintain rather than six exact sizes.
- **No macros in the public header** — nothing but the include guard and the `extern "C"` guard. Declarations are spelled out, so the API carries Doxygen comments and reads like the rest of `include/oqmc`.
- **Runtime sampler selection** — the implementation is chosen by name, so a consumer can expose the choice as a configuration setting without duplicating its render loop.
- **Self describing, position independent values** — the tag travels with the value, so it can be copied, packed, queued, serialised, or moved between processes.
- **Forward compatible storage** — reserved space allows a future sampler to grow without breaking already compiled downstream binaries.
- **One error path** — name validation happens once in `oqmc_sampler_type_find`; no other function reports an unknown sampler.
- **Two edit sites per sampler** — the existing C++ header plus one registry line. No public header change.
- **Value semantics preserved** — trivially copyable, no heap allocation, no destroy function.
- **Existing conventions preserved** — SPDX headers, `oqmc_` prefixed symbols, and the string named sampler dispatch already used by `oqmc_benchmark` and `oqmc_trace` in `src/tools/lib`.
