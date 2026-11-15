// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/// @file
/// @details C ABI for the sampler API. Included normally this is a C11 header
/// declaring a generic sampler type and functions.
///
/// When included with OQMC_IMPLEMENTATION defined, in exactly one C++17
/// translation unit, it also emits the implementation. The sampler wrapped
/// by the implementation is chosen when that translation unit is compiled, by
/// defining one of:
///
/// OQMC_CABI_SAMPLER_PMJ
/// OQMC_CABI_SAMPLER_PMJBN
/// OQMC_CABI_SAMPLER_SOBOL
/// OQMC_CABI_SAMPLER_SOBOLBN
/// OQMC_CABI_SAMPLER_LATTICE
/// OQMC_CABI_SAMPLER_LATTICEBN
///
/// With no definition it defaults to pmjbn.

#ifndef OQMC_OQMC_C_H
#define OQMC_OQMC_C_H

#include <stddef.h>
#include <stdint.h>

/// Exports the functions when OQMC_CABI_EXPORT_SYMBOLS is defined, such as
/// when building a shared library with hidden visibility.
#if defined(OQMC_CABI_EXPORT_SYMBOLS) && defined(_WIN32)
#define OQMC_CABI_EXPORT __declspec(dllexport)
#elif defined(OQMC_CABI_EXPORT_SYMBOLS)
#define OQMC_CABI_EXPORT __attribute__((visibility("default")))
#else
#define OQMC_CABI_EXPORT
#endif

/// Gives the functions C language linkage when compiled as C++.
#if defined(__cplusplus)
#define OQMC_CABI extern "C" OQMC_CABI_EXPORT
#else
#define OQMC_CABI OQMC_CABI_EXPORT
#endif

// NOLINTBEGIN: C style naming

/// Sampler value.
///
/// A trivially copyable value type. Copy it by assignment, pass it by
/// pointer, store it in arrays. There is no destroy function; a sampler
/// owns no resources. The storage is an implementation detail and should
/// not be read or written directly.
typedef struct oqmc_sampler
{
	uint64_t storage[2]; ///< Opaque storage for the sampler implementation.
} oqmc_sampler;

/// Name of the sampler implementation.
///
/// The implementation is chosen when the library is compiled. This lets an
/// application verify that it is running the sampler it expects.
///
/// @return Null terminated name, such as "pmjbn". Valid for the process.
OQMC_CABI const char* oqmc_sampler_name(void);

/// Required allocation size of the cache.
///
/// Prior to creating a sampler, a cache needs to be allocated and
/// initialised. This is the minimum required size in bytes of that
/// allocation. The allocation itself is performed and owned by the caller.
/// Responsibility for the de-allocation is also that of the caller.
///
/// @return Size of the cache in bytes. Can be zero.
OQMC_CABI size_t oqmc_sampler_cache_size(void);

/// Initialise the cache allocation.
///
/// Prior to creating a sampler, a cache needs to be allocated and
/// initialised. This function initialises that allocation. Once the cache is
/// initialised it may be used to create samplers, or copied to a new address.
///
/// A single cache is expected to be initialised only once for the duration of
/// the calling process. This single cache can be used to create many
/// samplers.
///
/// @param [in, out] cache Memory address of the cache allocation.
/// @pre Memory for the cache must be allocated prior. The minimum size of the
/// allocation can be retrieved using oqmc_sampler_cache_size.
/// @post Memory for the cache must be de-allocated after all samplers created
/// from it are no longer used.
OQMC_CABI void oqmc_sampler_initialise_cache(void* cache);

/// Create a sampler for a pixel.
///
/// Create a sampler based on the pixel, frame and sample indices. This also
/// requires an allocated and initialised cache. The resulting sampler is valid
/// and ready for use.
///
/// For each pixel this function is expected to be called multiple times, once
/// for each index. Pixels might have a varying number of indices when
/// adaptive sampling.
///
/// @param [in] x Pixel coordinate on the x axis.
/// @param [in] y Pixel coordinate on the y axis.
/// @param [in] frame Time index value.
/// @param [in] index Sample index. Must be positive.
/// @param [in] cache Allocated and initialised cache.
/// @return Sampler for the pixel domain.
/// @pre Cache has been allocated and initialised.
OQMC_CABI oqmc_sampler oqmc_sampler_create(int x, int y, int frame, int index,
                                           const void* cache);

/// Derive a sampler as a new domain.
///
/// The function derives a new sampler from the parent sampler. This new
/// sampler is called a domain. Each domain produces an independent 4
/// dimensional pattern. Calling the draw functions on the new child domain
/// will produce a different value to that of the parent domain.
///
/// N child domains can be derived from a single parent domain with the use
/// of the key argument. Keys must have at least one bit difference, but can
/// be a simple incrementing sequence. A single child domain can itself derive
/// N child domains. This process results in a domain tree.
///
/// The calling code can use up to 4 dimensions from each domain (these are
/// typically of the highest quality), joining them together to form an N
/// dimensional pattern. This technique is called padding.
///
/// @param [in] sampler Parent domain.
/// @param [in] key Index key of next domain.
/// @return Child domain based on the parent domain and key.
OQMC_CABI oqmc_sampler oqmc_sampler_new_domain(const oqmc_sampler* sampler,
                                               int key);

/// Derive a split sampler with a local and a global distribution.
///
/// Like oqmc_sampler_new_domain, this function derives a new domain. However,
/// using a technique called splitting, this domain can have a higher sample
/// rate based on a fixed multiplier.
///
/// The result from taking N indexed domains with this function will be both
/// a locally and a globally well distributed sub-pattern. This sub-pattern
/// will be of the highest quality due to being globally correlated with the
/// sub-patterns of other samples.
///
/// If a multiplier is adaptive (non-constant or unknown) then either the
/// oqmc_sampler_new_domain_distrib or oqmc_sampler_new_domain_chain functions
/// should be used instead. These methods relax the constraint for a fixed
/// multiplier, allowing for adaptive multipliers in exchange for a reduction
/// in the quality.
///
/// Calling code should use a constant key for any given domain while then
/// incrementing the index value N times to increase the sampling rate by N
/// for that given domain. The function will be called N times, once for each
/// unique index. N must be passed as size and remain constant.
///
/// @param [in] sampler Parent domain.
/// @param [in] key Index key of next domain.
/// @param [in] size Sample index multiplier. Must be greater than zero.
/// @param [in] index Sample index of next domain. Must be positive.
/// @return Child domain based on the parent domain, key and size.
OQMC_CABI oqmc_sampler oqmc_sampler_new_domain_split(
    const oqmc_sampler* sampler, int key, int size, int index);

/// Derive a split sampler with a local distribution.
///
/// Like oqmc_sampler_new_domain, this function derives a new domain. However,
/// using a technique called splitting, this domain can have a higher sample
/// rate based on an adaptive multiplier.
///
/// The result from taking N indexed domains with this function will be a
/// locally well distributed sub-pattern. This sub-pattern will be of lower
/// quality when combined with the sub-patterns of other samples. That is
/// because the correlation between the sub-patterns globally is lost.
///
/// If a multiplier is fixed (constant and known) then the
/// oqmc_sampler_new_domain_split function will produce better quality sample
/// points and should be used instead. This is because it will preserve
/// correlation between sub-patterns from other samples.
///
/// Calling code should use a constant key for any given domain while then
/// incrementing the index value N times to increase the sampling rate by N
/// for that given domain. The function will be called N times, once for each
/// unique index.
///
/// @param [in] sampler Parent domain.
/// @param [in] key Index key of next domain.
/// @param [in] index Sample index of next domain. Must be positive.
/// @return Child domain based on the parent domain and key.
OQMC_CABI oqmc_sampler oqmc_sampler_new_domain_distrib(
    const oqmc_sampler* sampler, int key, int index);

/// Derive a split sampler with a global distribution.
///
/// Like oqmc_sampler_new_domain, this function derives a new domain. However,
/// using a technique called splitting, this domain can have a higher sample
/// rate based on an adaptive multiplier.
///
/// The result from taking N indexed domains with this function will be a
/// globally well distributed sub-pattern. This sub-pattern will be of lower
/// quality when looking at the local correlation between different index
/// values. But, each index will be correlated globally.
///
/// If a multiplier is fixed (constant and known) then the
/// oqmc_sampler_new_domain_split function will produce better quality sample
/// points and should be used instead. This is because it will preserve
/// correlation between local points from different index values.
///
/// Calling code should use a constant key for any given domain while then
/// incrementing the index value N times to increase the sampling rate by N
/// for that given domain. The function will be called N times, once for each
/// unique index.
///
/// @param [in] sampler Parent domain.
/// @param [in] key Index key of next domain.
/// @param [in] index Sample index of next domain. Must be positive.
/// @return Child domain based on the parent domain and key.
OQMC_CABI oqmc_sampler
oqmc_sampler_new_domain_chain(const oqmc_sampler* sampler, int key, int index);

/// Draw integer sample values from the domain.
///
/// This can compute sample values with up to 4 dimensions for the given
/// domain. The operation does not change the state of the sampler, and for a
/// single domain and index, the result of this function will always be the
/// same. Output values are uniformly distributed integers within the range
/// of [0, 2^32).
///
/// These values are of high quality and should be handled with care as to
/// not introduce bias into an estimate. For low quality, but fast and safe
/// random numbers, use the oqmc_sampler_draw_rnd_u and
/// oqmc_sampler_draw_rnd_f functions.
///
/// @param [in] sampler Domain to draw from.
/// @param [in] size Number of dimensions to draw. Must be within [1, 4].
/// @param [out] sample Output array of at least size elements.
OQMC_CABI void oqmc_sampler_draw_sample_u(const oqmc_sampler* sampler, int size,
                                          uint32_t* sample);

/// Draw floating point sample values from the domain.
///
/// This function wraps oqmc_sampler_draw_sample_u. But transforms the output
/// values into uniformly distributed floats within the range of [0, 1).
///
/// @param [in] sampler Domain to draw from.
/// @param [in] size Number of dimensions to draw. Must be within [1, 4].
/// @param [out] sample Output array of at least size elements.
OQMC_CABI void oqmc_sampler_draw_sample_f(const oqmc_sampler* sampler, int size,
                                          float* sample);

/// Draw integer pseudo random values from the domain.
///
/// This can compute rnd values with up to 4 dimensions for the given domain.
/// The operation does not change the state of the sampler, and for a single
/// domain and index, the result of this function will always be the same.
/// Output values are uniformly distributed integers within the range of
/// [0, 2^32).
///
/// These values are of low quality but are fast to compute and have little
/// risk of biasing an estimate. For higher quality samples, use the
/// oqmc_sampler_draw_sample_u and oqmc_sampler_draw_sample_f functions.
///
/// @param [in] sampler Domain to draw from.
/// @param [in] size Number of dimensions to draw. Must be within [1, 4].
/// @param [out] rnd Output array of at least size elements.
OQMC_CABI void oqmc_sampler_draw_rnd_u(const oqmc_sampler* sampler, int size,
                                       uint32_t* rnd);

/// Draw floating point pseudo random values from the domain.
///
/// This function wraps oqmc_sampler_draw_rnd_u. But transforms the output
/// values into uniformly distributed floats within the range of [0, 1).
///
/// @param [in] sampler Domain to draw from.
/// @param [in] size Number of dimensions to draw. Must be within [1, 4].
/// @param [out] rnd Output array of at least size elements.
OQMC_CABI void oqmc_sampler_draw_rnd_f(const oqmc_sampler* sampler, int size,
                                       float* rnd);

// NOLINTEND

#endif // OQMC_OQMC_C_H

// The include guard above covers only the declarations, so that the header can
// be included for declarations any number of times, and for the
// implementation in exactly one translation unit.
#ifdef OQMC_IMPLEMENTATION

#ifndef __cplusplus
#error "OQMC_IMPLEMENTATION must be compiled as C++17."
#endif

#if defined(OQMC_CABI_SAMPLER_PMJ)
#include "pmj.h"
#elif defined(OQMC_CABI_SAMPLER_SOBOL)
#include "sobol.h"
#elif defined(OQMC_CABI_SAMPLER_SOBOLBN)
#include "sobolbn.h"
#elif defined(OQMC_CABI_SAMPLER_LATTICE)
#include "lattice.h"
#elif defined(OQMC_CABI_SAMPLER_LATTICEBN)
#include "latticebn.h"
#else
#include "pmjbn.h"
#endif

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace
{

/// @cond

#if defined(OQMC_CABI_SAMPLER_PMJ)
using Sampler = oqmc::PmjSampler;
constexpr const char* samplerName = "pmj";
#elif defined(OQMC_CABI_SAMPLER_SOBOL)
using Sampler = oqmc::SobolSampler;
constexpr const char* samplerName = "sobol";
#elif defined(OQMC_CABI_SAMPLER_SOBOLBN)
using Sampler = oqmc::SobolBnSampler;
constexpr const char* samplerName = "sobolbn";
#elif defined(OQMC_CABI_SAMPLER_LATTICE)
using Sampler = oqmc::LatticeSampler;
constexpr const char* samplerName = "lattice";
#elif defined(OQMC_CABI_SAMPLER_LATTICEBN)
using Sampler = oqmc::LatticeBnSampler;
constexpr const char* samplerName = "latticebn";
#else
using Sampler = oqmc::PmjBnSampler;
constexpr const char* samplerName = "pmjbn";
#endif

static_assert(sizeof(oqmc_sampler) == 16, "Value must be 16 bytes in size.");
static_assert(alignof(oqmc_sampler) == 8, "Value must have 8 byte alignment.");
static_assert(sizeof(Sampler) <= sizeof(oqmc_sampler),
              "Sampler must fit within the value storage.");
static_assert(alignof(Sampler) <= alignof(oqmc_sampler),
              "Sampler alignment must not exceed the value alignment.");

Sampler load(const oqmc_sampler* sampler)
{
	Sampler out;
	// Cast to void* since GCC warns on memcpy into a class with private
	// members; the samplers are trivially copyable, so the copy is valid.
	std::memcpy(static_cast<void*>(&out), sampler, sizeof(out));

	return out;
}

oqmc_sampler store(const Sampler& sampler)
{
	oqmc_sampler out{};
	std::memcpy(&out, &sampler, sizeof(sampler));

	return out;
}

template <typename T>
void drawSample(const Sampler& sampler, int size, T* sample)
{
	assert(size >= 1 && size <= 4);

	switch(size)
	{
	case 1:
		sampler.drawSample<1>(sample);
		break;
	case 2:
		sampler.drawSample<2>(sample);
		break;
	case 3:
		sampler.drawSample<3>(sample);
		break;
	case 4:
		sampler.drawSample<4>(sample);
		break;
	default:
		break;
	}
}

template <typename T>
void drawRnd(const Sampler& sampler, int size, T* rnd)
{
	assert(size >= 1 && size <= 4);

	switch(size)
	{
	case 1:
		sampler.drawRnd<1>(rnd);
		break;
	case 2:
		sampler.drawRnd<2>(rnd);
		break;
	case 3:
		sampler.drawRnd<3>(rnd);
		break;
	case 4:
		sampler.drawRnd<4>(rnd);
		break;
	default:
		break;
	}
}

/// @endcond

} // namespace

OQMC_CABI const char* oqmc_sampler_name(void)
{
	return samplerName;
}

OQMC_CABI size_t oqmc_sampler_cache_size(void)
{
	return Sampler::cacheSize;
}

OQMC_CABI void oqmc_sampler_initialise_cache(void* cache)
{
	Sampler::initialiseCache(cache);
}

OQMC_CABI oqmc_sampler oqmc_sampler_create(int x, int y, int frame, int index,
                                           const void* cache)
{
	return store(Sampler(x, y, frame, index, cache));
}

OQMC_CABI oqmc_sampler oqmc_sampler_new_domain(const oqmc_sampler* sampler,
                                               int key)
{
	return store(load(sampler).newDomain(key));
}

OQMC_CABI oqmc_sampler oqmc_sampler_new_domain_split(
    const oqmc_sampler* sampler, int key, int size, int index)
{
	return store(load(sampler).newDomainSplit(key, size, index));
}

OQMC_CABI oqmc_sampler
oqmc_sampler_new_domain_distrib(const oqmc_sampler* sampler, int key, int index)
{
	return store(load(sampler).newDomainDistrib(key, index));
}

OQMC_CABI oqmc_sampler
oqmc_sampler_new_domain_chain(const oqmc_sampler* sampler, int key, int index)
{
	return store(load(sampler).newDomainChain(key, index));
}

OQMC_CABI void oqmc_sampler_draw_sample_u(const oqmc_sampler* sampler, int size,
                                          uint32_t* sample)
{
	drawSample(load(sampler), size, sample);
}

OQMC_CABI void oqmc_sampler_draw_sample_f(const oqmc_sampler* sampler, int size,
                                          float* sample)
{
	drawSample(load(sampler), size, sample);
}

OQMC_CABI void oqmc_sampler_draw_rnd_u(const oqmc_sampler* sampler, int size,
                                       uint32_t* rnd)
{
	drawRnd(load(sampler), size, rnd);
}

OQMC_CABI void oqmc_sampler_draw_rnd_f(const oqmc_sampler* sampler, int size,
                                       float* rnd)
{
	drawRnd(load(sampler), size, rnd);
}

#endif // OQMC_IMPLEMENTATION
