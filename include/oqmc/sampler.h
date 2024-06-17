// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/**
 * @file
 * @details Sampler interface definition.
 */

#pragma once

#include "float.h" // NOLINT: false positive
#include "gpu.h"
#include "range.h"

#include <cassert>
#include <cstddef>
#include <cstdint>

/**
 * @defgroup samplers Sampler API
 * @brief Higher level sampler API and sampler types.
 * @details This module outlines the higher level sampler API, as well as each
 * availble sampler type. Sampler type implementations are non-public, and all
 * functionality is accessible via the SamplerInterface. Sampler types and a
 * corresponding header file are listed under Typedef Documentation.
 *
 * **Blue noise sampler variants**
 *
 * There are typically two varaints of each sampler type, a base variant and a
 * blue noise variant. Each blue noise variant is constructed using an offline
 * optimisation process that is based on the work by Belcour and Heitz in
 * 'Lessons Learned and Improvements when Building Screen-Space Samplers with
 * Blue-Noise Error Distribution', and extends temporally as described by Wolfe
 * et al. in 'Spatiotemporal Blue Noise Masks'.
 *
 * Blue noise variants achieve a blue noise distribution using two pixel tables,
 * one holds keys to seed the sequence, and the other index ranks. These tables
 * are then randomised by toroidally shifting the table lookups for each domain
 * using random offsets. Correlation between the offsets and the pixels allows
 * for a single pair of tables to provide keys and ranks for all domains.
 *
 * Although the spatial temporal blue noise does not reduce the error for
 * an individual pixel, it does give a better perceptual result due to less
 * low frequency structure in the error between pixels. Also, if an image
 * is either spatially or temporally filtered (as with denoising or temporal
 * anti-aliasing), the resulting error can be lower when using a blue noise
 * variant.
 *
 * Blue noise variants are recommended over the base variants of each sampler,
 * as the additional performance cost will most likely be minimal in relation
 * to the gains to be had at low sample counts. However, the access to the data
 * tables could have a larger impact on performance depending on the artchecture
 * (GPU, etc), so it is worth benchmarking if this is a concern.
 *
 * **Packing and passing samplers**
 *
 * The intention of the API is to allow for object instances to be efficiently
 * copied on the stack. In doing so the objects can be easily packed and queued
 * during wavefront rendering. Allowing for this means that along with the
 * possible 64 bits to track a cache, the state that uniquely identifies the
 * current domain and index of a sampler needs to be small as well.
 *
 * For this purpose, each implementation uses 64 bits. Along with an optional
 * cache pointer, that gives a maximum of 128 bits for the entire object.
 * Creating new states for each domain relies heavily on PCG-RXS-M-RX-32 from
 * the PCG family of PRNGs as described by O'Neill in 'PCG: A Family of Simple
 * Fast Space-Efficient Statistically Good Algorithms for Random Number
 * Generation'.
 *
 * Each implementation uses the LCG state transition when advancing a domain,
 * but then increases the quality of the lower order bits with a permutation
 * prior to drawing samples. This allows the implementation to efficiently
 * create and skip domains, while producing high quality random results upon
 * drawing samples.
 */

namespace oqmc
{

/**
 * @brief Sampler API.
 * @details This is a sampler interface that defines a generic API for all
 * sampler types. The interface is composed of an internal implementation,
 * meaning only this public API is exposed to calling code.
 *
 * @internal
 * New implementations should define an implementation type, and pass this type
 * as a template parameter to an instantiation of SamplerInterface.
 *
 * @code
 * class FooImpl
 * {
 * ...
 * }
 *
 * using FooSampler = SamplerInterface<FooImpl>;
 * @endcode
 *
 * Samplers should aim to have a small memory footprint, and be cheap to copy,
 * allowing copy by value when passing as a function argument.
 * @endinternal
 *
 * Different samplers defined using the interface should be interchangeable
 * allowing for new implementations to be tested and compared without changing
 * the calling code. The interface is static, so all functions should be inlined
 * to allow for zero cost abstraction. This also means that enabling compile
 * time optimisations might provide a noticable improvement.
 *
 * Samplers can only be constructed, their state cannot change. In most cases
 * the object variable can be marked constant. New samplers are created from
 * a parent sampler using the newDomain member functions. Sample values are
 * retrieved using the draw member functions. Calls to newDomain functions
 * should be cheap in comparison to calls to draw functions.
 *
 * @ingroup samplers
 *
 * @tparam Impl Internal sampler implementation type as described above.
 */
template <typename Impl>
class SamplerInterface
{
	// Dimensions per pattern.
	static constexpr auto maxDrawValue = 4;

	// Prevent value-construction.
	OQMC_HOST_DEVICE SamplerInterface(Impl impl);

	// Implemention type.
	Impl impl;

  public:
	/**
	 * @brief Required allocation size of the cache.
	 * @details Prior to construction of a sampler object, a cache needs to be
	 * allocated and initialised for any given sampler type. This variable is
	 * the minimum required size in bytes of that allocation. The allocation
	 * itself is performed and owned by the caller. Responsibility for the
	 * de-allocation is also that of the caller.
	 */
	static constexpr std::size_t cacheSize = Impl::cacheSize;

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
	static void initialiseCache(void* cache);

	/**
	 * @brief Construct an invalid object.
	 * @details Create a placeholder object to allocate containers, etc. The
	 * resulting object is invalid, and you should initialise it by replacing
	 * the object with another from a parametrised constructor.
	 */
	/*AUTO_DEFINED*/ SamplerInterface() = default;

	/**
	 * @brief Parametrised pixel constructor.
	 * @details Create an object based on the pixel, frame and sample indices.
	 * This also requires a pre-allocated and initialised cache. Once
	 * constructed the object is valid and ready for use.
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
	OQMC_HOST_DEVICE SamplerInterface(int x, int y, int frame, int index,
	                                  const void* cache);

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
	OQMC_HOST_DEVICE SamplerInterface newDomain(int key) const;

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
	OQMC_HOST_DEVICE SamplerInterface newDomainDistrib(int key,
	                                                   int index) const;

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
	OQMC_HOST_DEVICE SamplerInterface newDomainSplit(int key, int size,
	                                                 int index) const;

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
	OQMC_HOST_DEVICE void drawSample(std::uint32_t sample[Size]) const;

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
	OQMC_HOST_DEVICE void drawSample(std::uint32_t range,
	                                 std::uint32_t sample[Size]) const;

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
	OQMC_HOST_DEVICE void drawSample(float sample[Size]) const;

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
	OQMC_HOST_DEVICE void drawRnd(std::uint32_t rnd[Size]) const;

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
	OQMC_HOST_DEVICE void drawRnd(std::uint32_t range,
	                              std::uint32_t rnd[Size]) const;

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
	OQMC_HOST_DEVICE void drawRnd(float rnd[Size]) const;
};

template <typename Impl>
void SamplerInterface<Impl>::initialiseCache(void* cache)
{
	Impl::initialiseCache(cache);
}

template <typename Impl>
SamplerInterface<Impl>::SamplerInterface(Impl impl) : impl(impl)
{
}

template <typename Impl>
SamplerInterface<Impl>::SamplerInterface(int x, int y, int frame, int index,
                                         const void* cache)
    : impl(x, y, frame, index, cache)
{
	assert(index >= 0);
}

template <typename Impl>
SamplerInterface<Impl> SamplerInterface<Impl>::newDomain(int key) const
{
	return {impl.newDomain(key)};
}

template <typename Impl>
SamplerInterface<Impl> SamplerInterface<Impl>::newDomainDistrib(int key,
                                                                int index) const
{
	assert(index >= 0);

	return {impl.newDomainDistrib(key, index)};
}

template <typename Impl>
SamplerInterface<Impl> SamplerInterface<Impl>::newDomainSplit(int key, int size,
                                                              int index) const
{
	assert(size > 0);
	assert(index >= 0);

	return {impl.newDomainSplit(key, size, index)};
}

template <typename Impl>
template <int Size>
void SamplerInterface<Impl>::drawSample(std::uint32_t sample[Size]) const
{
	static_assert(Size >= 0, "Draw size greater or equal to zero.");
	static_assert(Size <= maxDrawValue, "Draw size less or equal to max.");

	impl.template drawSample<Size>(sample);
}

template <typename Impl>
template <int Size>
void SamplerInterface<Impl>::drawSample(std::uint32_t range,
                                        std::uint32_t sample[Size]) const
{
	assert(range > 0);

	std::uint32_t integerSample[Size];
	drawSample<Size>(integerSample);

	for(int i = 0; i < Size; ++i)
	{
		sample[i] = uintToRange(integerSample[i], range);
	}
}

template <typename Impl>
template <int Size>
void SamplerInterface<Impl>::drawSample(float sample[Size]) const
{
	std::uint32_t integerSample[Size];
	drawSample<Size>(integerSample);

	for(int i = 0; i < Size; ++i)
	{
		sample[i] = uintToFloat(integerSample[i]);
	}
}

template <typename Impl>
template <int Size>
void SamplerInterface<Impl>::drawRnd(std::uint32_t rnd[Size]) const
{
	static_assert(Size >= 0, "Draw size greater or equal to zero.");
	static_assert(Size <= maxDrawValue, "Draw size less or equal to max.");

	impl.template drawRnd<Size>(rnd);
}

template <typename Impl>
template <int Size>
void SamplerInterface<Impl>::drawRnd(std::uint32_t range,
                                     std::uint32_t rnd[Size]) const
{
	assert(range > 0);

	std::uint32_t integerRnd[Size];
	drawRnd<Size>(integerRnd);

	for(int i = 0; i < Size; ++i)
	{
		rnd[i] = uintToRange(integerRnd[i], range);
	}
}

template <typename Impl>
template <int Size>
void SamplerInterface<Impl>::drawRnd(float rnd[Size]) const
{
	std::uint32_t integerRnd[Size];
	drawRnd<Size>(integerRnd);

	for(int i = 0; i < Size; ++i)
	{
		rnd[i] = uintToFloat(integerRnd[i]);
	}
}

} // namespace oqmc
