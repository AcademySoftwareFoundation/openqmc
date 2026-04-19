# Option B: Runtime Sampler Selection

Detail for Option B of the [C ABI specification](spec.md). This design exposes
the generic C API defined in the specification, with all samplers compiled
into the library and one selected at startup, defaulting to PmjBn if no
selection is made. Every call dispatches through a single global pointer to a
table of function pointers held in read-only data.

Note that the illustrative code below needs correction before use; it has not
been compiled.

## Selection API

The public header adds one enumeration and one function to the generic API.

```c
/// Sampler implementation.
typedef enum oqmc_sampler_type
{
	OQMC_SAMPLER_TYPE_PMJ,          ///< Low discrepancy pmj sequence.
	OQMC_SAMPLER_TYPE_PMJBN,        ///< Blue noise pmj variant.
	OQMC_SAMPLER_TYPE_SOBOL,        ///< Owen scrambled Sobol sequence.
	OQMC_SAMPLER_TYPE_SOBOLBN,      ///< Blue noise Sobol variant.
	OQMC_SAMPLER_TYPE_LATTICE,      ///< Rank one lattice sequence.
	OQMC_SAMPLER_TYPE_LATTICEBN,    ///< Blue noise lattice variant.
} oqmc_sampler_type;

/// Select the sampler implementation for this process.
///
/// Optional; the default is pmjbn. Call at most once, at startup, before any
/// other API function and before starting any thread that uses the API.
/// Calling it later is undefined.
///
/// @param [in] type Implementation to select.
/// @return Zero on success, non-zero if the type is invalid.
int oqmc_sampler_select(oqmc_sampler_type type);
```

The enumerator values are the indices into the internal registry, so
reordering either is an ABI break; new samplers are appended. A consumer
selecting by name, from a command line or a configuration file, maps the
string to an enumerator itself; `oqmc_sampler_select` validates the result
either way.

**Selection is a startup operation**: sampler values carry no type
information, so changing the implementation while values are live would
reinterpret them under a different layout, and a select after other API use
is therefore undefined. This is a precondition in the same style as the rest
of the library, not something the implementation detects. Components sharing
one central library, such as plugins in a host process, must agree on the
sampler, which `oqmc_sampler_name` can check. One sampler per process is a
permanent commitment of this design.

## Implementation

Templates generate the per-sampler code. A `Vtable` holds one slot per API
function, a `Thunks` template translates between the opaque value and a
concrete C++ sampler, and a `constexpr` registry maps enumerators to tables.
Two static assertions per sampler make an oversized or non-trivial sampler a
compile error.

```cpp
struct Vtable
{
	const char* name;
	size_t cacheSize;
	void (*initialiseCache)(void* cache);
	void (*create)(int x, int y, int frame, int index, const void* cache,
	               oqmc_sampler* out);
	void (*newDomain)(const oqmc_sampler* self, int key, oqmc_sampler* out);
	// One slot per remaining API function.
};

template <typename Sampler> struct Thunks
{
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

	static void store(oqmc_sampler* out, const Sampler& sampler)
	{
		std::memcpy(out, &sampler, sizeof(sampler));
	}

	static void newDomain(const oqmc_sampler* self, int key, oqmc_sampler* out)
	{
		store(out, load(self).newDomain(key));
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

	// Remaining members follow the same pattern.
};
```

The registry is a compile time constant, so the tables live in read-only data
with no runtime initialisation, and a static assertion keeps it aligned with
the public enumeration:

```cpp
template <typename Sampler> constexpr Vtable makeVtable(const char* name)
{
	return Vtable{name,
	              Sampler::cacheSize,
	              &Thunks<Sampler>::initialiseCache,
	              &Thunks<Sampler>::create,
	              &Thunks<Sampler>::newDomain,
	              // Remaining slots follow the same pattern.
	};
}

constexpr Vtable registry[] = {
    makeVtable<oqmc::PmjSampler>("pmj"),
    makeVtable<oqmc::PmjBnSampler>("pmjbn"),
    makeVtable<oqmc::SobolSampler>("sobol"),
    makeVtable<oqmc::SobolBnSampler>("sobolbn"),
    makeVtable<oqmc::LatticeSampler>("lattice"),
    makeVtable<oqmc::LatticeBnSampler>("latticebn"),
};

constexpr int registryCount = int(std::size(registry));
static_assert(registryCount == OQMC_SAMPLER_TYPE_LATTICEBN + 1,
              "Registry and enumeration must match.");
```

The selection is one pointer, statically initialised to the default table.
It is written at most once, at startup, and only read after that, so there is
no data race and a plain pointer is sufficient. Every call pays one load and
an indirect call.

```cpp
const Vtable* active = &registry[OQMC_SAMPLER_TYPE_PMJBN];

int oqmc_sampler_select(oqmc_sampler_type type)
{
	if(type < 0 || type >= registryCount)
	{
		return 1;
	}

	active = &registry[type];

	return 0;
}
```

Every entry point then follows one shape:

```cpp
oqmc_sampler oqmc_sampler_new_domain(const oqmc_sampler* sampler, int key)
{
	oqmc_sampler out{};
	active->newDomain(sampler, key, &out);

	return out;
}
```

## Contributor Workflow

Adding a new sampler requires the existing C++ header, plus two appended
lines: an enumerator in `oqmc_c.h` and a registry entry in `cabi.cpp`.

```cpp
makeVtable<oqmc::MySampler>("my"),
```

The static assertion ties the two together, so forgetting one of them is a
compile error rather than a drift.

## Usage Example

```c
#include <oqmc/oqmc_c.h>

#include <stdlib.h>

int main(void)
{
	/* Override the pmjbn default for this process. */
	if(oqmc_sampler_select(OQMC_SAMPLER_TYPE_SOBOL) != 0)
	{
		return 1;
	}

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

## Testing

In addition to the common tests in the specification:

- **Selection** — an invalid type is rejected; with no select call the API
  draws values matching the C++ PmjBn sampler; after a select it matches the
  selected sampler.
- Because selection happens once per process, equivalence tests covering
  more than one sampler must run each sampler in its own process, for
  example by invoking the test binary once per sampler name.

## Key Properties

- **One prebuilt binary** — a single package serves every consumer, with the
  sampler chosen from configuration at startup, no rebuild.
- **Same value type as Option A** — 16 bytes, register return; the type
  information lives in one global rather than in every value.
- **Works with zero configuration** — with no select call the library is the
  recommended pmjbn sampler; select exists to override it at startup.
- **Dispatch within noise** — one load and an indirect call, measured
  indistinguishable from direct calls at the library boundary.
- **Two edit sites per new sampler** — one enumerator, one registry entry,
  tied together by a static assertion.
- **The trade** — every build links all samplers and their data tables, one
  process-wide global exists, and one sampler per process is a permanent
  commitment.
