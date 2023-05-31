// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

/**
 * @file
 * @details Sampler interface definition.
 */

#pragma once

#include "float.h" // NOLINT: false positive
#include "gpu.h"

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace oqmc
{

/**
 * @brief Sampler interface wrapper.
 * @details This is the sampler interface that defines the generic API for
 * sampler types. The implementation is composed internally, allowing only the
 * API to be exposed to the calling code, establishing a strict contract. New
 * implementations should define a type, passing the internal implementation
 * class as a template parameter. An implementation definition looks like:
 *
 * #include "gpu.h"
 * #include "sampler.h"
 *
 * #include <cstddef>
 * #include <cstdint>
 *
 * class SamplerNameImpl
 * {
 *   friend SamplerInterface<SamplerNameImpl>;
 *
 *   static constexpr std::size_t cacheSize = 0;
 *   static void initialiseCache(void* cache);
 *
 *   SamplerNameImpl() = default;
 *   OQMC_HOST_DEVICE SamplerNameImpl(int x, int y, int frame, int sampleId,
 *                                    const void* cache);
 *
 *   OQMC_HOST_DEVICE SamplerNameImpl newDomain(int key) const;
 *   OQMC_HOST_DEVICE SamplerNameImpl newDomainDistrib(int key) const;
 *   OQMC_HOST_DEVICE SamplerNameImpl newDomainSplit(int key, int size) const;
 *   OQMC_HOST_DEVICE SamplerNameImpl nextDomainIndex() const;
 *
 *   template <int Size>
 *   OQMC_HOST_DEVICE void drawSample(std::uint32_t samples[Size]) const;
 *
 *   template <int Size>
 *   OQMC_HOST_DEVICE void drawRnd(std::uint32_t rnds[Size]) const;
 * };
 *
 * using SamplerNameSampler = SamplerInterface<SamplerNameImpl>;
 *
 * Different samplers defined using the interface should be interchangeable
 * allowing for new implementations to be tested and compared without changing
 * the calling code. The interface is static, so all functions should be inlined
 * with zero cost abstraction. This also means that enabling compile time
 * optimisations is critical for performance.
 *
 * Once a sampler is constructed its state cannot change. This means that in
 * most cases the object variable can be marked constant. New samplers are
 * created from a parent sampler using the newDomain* member functions. Sample
 * values are retrieved using the draw* member functions. Calls to newDomain*
 * functions should be cheap in comparison to calls to draw* functions.
 *
 * Samplers should aim to have a small memory footprint, and be cheap to copy,
 * allowing copy by value when passing as a function argument.
 *
 * @tparam Impl Internal sampler implementation type as described above.
 */
template <typename Impl>
class SamplerInterface
{
	static constexpr auto maxDrawValue = 4;       // dimensions per pattern.
	static constexpr auto maxIndexSize = 0x10000; // 2^16 index upper limit.

	OQMC_HOST_DEVICE SamplerInterface(Impl impl);

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
	 * Values for sampleId should be within the [0, 2^16) range; this constraint
	 * allows for possible optimisations in the implementations.
	 *
	 * @param x [in] Pixel coordinate on the x axis.
	 * @param y [in] Pixel coordinate on the y axis.
	 * @param frame [in] Time index value.
	 * @param sampleId [in] Sample index. Must be within [0, 2^16).
	 * @param cache [in] Allocated and initialised cache.
	 *
	 * @pre Cache has been allocated in memory accessible to the device calling
	 * this constructor, and has also been initialised.
	 */
	OQMC_HOST_DEVICE SamplerInterface(int x, int y, int frame, int sampleId,
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
	 * with the sample index, allowing for sample splitting with an unknown
	 * multiplier.
	 *
	 * Calling code that needs to take N branching samples can't simply call the
	 * nextDomainIndex member function shown below. This would cause duplication
	 * with other domain trees, and ultimately bias the estimate. By deriving a
	 * decorrelated domain using this member function, the calling code can
	 * safely iterate N samples from the resulting child domain.
	 *
	 * The resulting pattern from taking multiple samples with nextDomainIndex
	 * will be well distributed locally. But care must be taken as this local
	 * pattern will be of lower quality when combined with the local patterns of
	 * other samples. As the correlation between the patterns globally has been
	 * lost. If the multiplier (the amount of times nextDomainIndex is to be
	 * called) is known and constant, newDomainSplit can be used instead to
	 * produce the best quality results.
	 *
	 * @param [in] key Index key of next domain.
	 * @return Child domain based on the current object state and key.
	 */
	OQMC_HOST_DEVICE SamplerInterface newDomainDistrib(int key) const;

	/**
	 * @brief Derive an object in a new domain for splitting.
	 * @details Like the other newDomain* functions, this function derives a
	 * mutated copy of the current sampler object. The difference is it remaps
	 * the sample index, allowing for sample splitting with a known and constant
	 * multiplier.
	 *
	 * Calling code that needs to take N branching samples can't simply call the
	 * nextDomainIndex member function shown below. This would cause duplication
	 * with other domain trees, and ultimately bias the estimate. By deriving a
	 * new domain using this member function, the calling code can safely
	 * iterate N samples from the resulting child domain.
	 *
	 * As the multiplier is a known and constant size, not only will the
	 * resulting pattern from taking multiple samples with nextDomainIndex be
	 * well distributed locally, but it will also be well distributed globally.
	 * This is because the indices can be mapped more carefully when the size of
	 * N is known and does not change. If these guarantees can not be met then
	 * the calling code should use newDomainDistrib instead.
	 *
	 * @param [in] key Index key of next domain.
	 * @param [in] size Sample index multiplier. Must be greater than zero.
	 * @return Child domain based on the current object state, key and size.
	 */
	OQMC_HOST_DEVICE SamplerInterface newDomainSplit(int key, int size) const;

	/**
	 * @brief Derive an object in the current domain at the next index.
	 * @details The function derives a mutated copy of the current sampler
	 * object. This new object is in the same domain, but will have iterated
	 * onto the next sample index. Calling the draw* member functions below on
	 * the new index will produce a different value to that of the previous
	 * index.
	 *
	 * This is used to split a single sample index into multiple indices so that
	 * a subset of an integrals dimensions can sampled at higher rate than other
	 * dimensions. This technique is called sample or trajectory splitting.
	 *
	 * The function should only be called on domains that have been configured
	 * for splitting, else it would cause duplication with other domain trees,
	 * and ultimately bias the estimate. If newDomainDistrib was used, then the
	 * only restriction is that the resulting sample index must not exceed the
	 * global limit of 2^16. If newDomainSplit was used, then the calling code
	 * must iterate with this function as many times as was specified in the
	 * 'size' argument.
	 *
	 * @return Child sampler object based on current state.
	 *
	 * @pre The current object must be the result of calling either the
	 * newDomainDistrib or newDomainSplit member functions.
	 */
	OQMC_HOST_DEVICE SamplerInterface nextDomainIndex() const;

	/**
	 * @brief Draw integer sample values from domain
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
	 * @param [out] samples Output array to store sample values.
	 */
	template <int Size>
	OQMC_HOST_DEVICE void drawSample(std::uint32_t samples[Size]) const;

	/**
	 * @brief Draw floating point sample values from domain
	 * @details This function wraps the integer variant of drawSample above. But
	 * transforms the output values into uniformly distributed floats within the
	 * range of [0, 1).
	 *
	 * @tparam Size Number of dimensions to draw. Must be within [1, 4].
	 *
	 * @param [out] samples Output array to store sample values.
	 */
	template <int Size>
	OQMC_HOST_DEVICE void drawSample(float samples[Size]) const;

	/**
	 * @brief Draw integer pseudo random values from domain
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
	 * @param [out] samples Output array to store sample values.
	 */
	template <int Size>
	OQMC_HOST_DEVICE void drawRnd(std::uint32_t rnds[Size]) const;

	/**
	 * @brief Draw floating point pseudo random values from domain
	 * @details This function wraps the integer variant of drawRnd above. But
	 * transforms the output values into uniformly distributed floats within the
	 * range of [0, 1).
	 *
	 * @tparam Size Number of dimensions to draw. Must be within [1, 4].
	 *
	 * @param [out] samples Output array to store sample values.
	 */
	template <int Size>
	OQMC_HOST_DEVICE void drawRnd(float rnds[Size]) const;
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
SamplerInterface<Impl>::SamplerInterface(int x, int y, int frame, int sampleId,
                                         const void* cache)
    : impl(x, y, frame, sampleId, cache)
{
	assert(sampleId >= 0);
	assert(sampleId < maxIndexSize);
}

template <typename Impl>
SamplerInterface<Impl> SamplerInterface<Impl>::newDomain(int key) const
{
	return {impl.newDomain(key)};
}

template <typename Impl>
SamplerInterface<Impl> SamplerInterface<Impl>::newDomainDistrib(int key) const
{
	return {impl.newDomainDistrib(key)};
}

template <typename Impl>
SamplerInterface<Impl> SamplerInterface<Impl>::newDomainSplit(int key,
                                                              int size) const
{
	assert(size > 0);

	return {impl.newDomainSplit(key, size)};
}

template <typename Impl>
SamplerInterface<Impl> SamplerInterface<Impl>::nextDomainIndex() const
{
	return {impl.nextDomainIndex()};
}

template <typename Impl>
template <int Size>
void SamplerInterface<Impl>::drawSample(std::uint32_t samples[Size]) const
{
	static_assert(Size >= 0, "Draw size greater or equal to zero.");
	static_assert(Size <= maxDrawValue, "Draw size less or equal to max.");

	impl.template drawSample<Size>(samples);
}

template <typename Impl>
template <int Size>
void SamplerInterface<Impl>::drawSample(float samples[Size]) const
{
	std::uint32_t integerSamples[Size];
	drawSample<Size>(integerSamples);

	for(int i = 0; i < Size; ++i)
	{
		samples[i] = uintToFloat(integerSamples[i]);
	}
}

template <typename Impl>
template <int Size>
void SamplerInterface<Impl>::drawRnd(std::uint32_t rnds[Size]) const
{
	static_assert(Size >= 0, "Draw size greater or equal to zero.");
	static_assert(Size <= maxDrawValue, "Draw size less or equal to max.");

	impl.template drawRnd<Size>(rnds);
}

template <typename Impl>
template <int Size>
void SamplerInterface<Impl>::drawRnd(float rnds[Size]) const
{
	std::uint32_t integerRnds[Size];
	drawRnd<Size>(integerRnds);

	for(int i = 0; i < Size; ++i)
	{
		rnds[i] = uintToFloat(integerRnds[i]);
	}
}

} // namespace oqmc
