// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdlib>

#if defined(__CUDACC__)
#include <cstdio>

inline void handleError(cudaError_t error, const char* file, int line)
{
	assert(file);

	if(error == cudaSuccess)
	{
		return;
	}

	fprintf(stderr, "\nCUDA: %s; %s:%d\n", cudaGetErrorString(error), file,
	        line);

	exit(EXIT_FAILURE);
}

template <typename T>
cudaError_t allocate(T** ret, size_t size)
{
	assert(ret);

	if(size == 0)
	{
		*ret = nullptr;
		return cudaSuccess;
	}

	return cudaMallocManaged(ret, sizeof(T) * size);
}

template <>
inline cudaError_t allocate(void** ret, size_t size)
{
	assert(ret);

	if(size == 0)
	{
		*ret = nullptr;
		return cudaSuccess;
	}

	return cudaMallocManaged(ret, size);
}

template <typename Func>
__global__ void kernel(Func func, size_t begin, size_t end)
{
	const size_t index = blockIdx.x * blockDim.x + threadIdx.x;
	const size_t stride = blockDim.x * gridDim.x;

	for(auto i = begin + index; i < end; i += stride)
	{
		func(i);
	}
}

#define OQMC_HANDLE_ERROR(ERROR) handleError(ERROR, __FILE__, __LINE__)
#define OQMC_ALLOCATE(PTR, SIZE) OQMC_HANDLE_ERROR(allocate(PTR, SIZE))
#define OQMC_FREE(PTR) OQMC_HANDLE_ERROR(cudaFree(PTR))
#define OQMC_LAUNCH(kernel, ...)                                               \
	kernel<<<8, 128>>>(__VA_ARGS__);                                           \
	OQMC_HANDLE_ERROR(cudaGetLastError());                                     \
	OQMC_HANDLE_ERROR(cudaDeviceSynchronize())
#define OQMC_FORLOOP(func, begin, end)                                         \
	kernel<<<((end - begin) + 256 - 1) / 256, 256>>>(func, begin, end);        \
	OQMC_HANDLE_ERROR(cudaGetLastError());                                     \
	OQMC_HANDLE_ERROR(cudaDeviceSynchronize())
#define OQMC_MEMCPY(DEST, SRC, COUNT)                                          \
	OQMC_HANDLE_ERROR(cudaMemcpy(DEST, SRC, COUNT, cudaMemcpyDeviceToDevice))
#else
#include <cstring>
#include <oneapi/tbb.h>

template <typename T>
void allocate(T** ret, size_t size)
{
	assert(ret);

	*ret = static_cast<T*>(malloc(sizeof(T) * size));
}

template <>
inline void allocate(void** ret, size_t size)
{
	assert(ret);

	*ret = malloc(size);
}

template <typename Func>
void kernel(Func func, size_t begin, size_t end)
{
	const auto range = oneapi::tbb::blocked_range<size_t>(begin, end);
	const auto loop = [func](const oneapi::tbb::blocked_range<size_t>& r) {
		for(auto i = r.begin(); i != r.end(); ++i)
		{
			func(i);
		}
	};

	oneapi::tbb::parallel_for(range, loop);
}

#define OQMC_ALLOCATE(PTR, SIZE) allocate(PTR, SIZE)
#define OQMC_FREE(PTR) free(PTR)
#define OQMC_LAUNCH(kernel, ...) kernel(__VA_ARGS__)
#define OQMC_FORLOOP(func, begin, end) kernel(func, begin, end)
#define OQMC_MEMCPY(DEST, SRC, COUNT) std::memcpy(DEST, SRC, COUNT)
#endif
