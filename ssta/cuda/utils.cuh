#include <iostream>
#include <cassert>

#ifndef _UTIL_CUH
#define _UTIL_CUH

#define checkCUDA(status) \
{\
	if (status != cudaSuccess) { \
		printf("CUDA Runtime Error: %s\n", \
			cudaGetErrorString(status)); \
		assert(status == cudaSuccess); \
	} \
}

#define allocateCUDA(var, size, type) \
{                                                                     \
  cudaError_t status = cudaMalloc(&(var), (size)*sizeof(type));       \
  if (status != cudaSuccess)                                          \
  {                                                                   \
    printf("cudaMalloc failed for var " #var " with size %zu\n", (size_t)size); \
    printf("CUDA Runtime Error: %s\n",                                \
            cudaGetErrorString(status));                               \
    assert(status == cudaSuccess);                                    \
  }                                                                   \
}

#define allocateAsyncCUDA(var, size, type, stream) \
{                                                                             \
  cudaError_t status = cudaMallocAsync(&(var), (size)*sizeof(type), stream);  \
  if (status != cudaSuccess)                                                  \
  {                                                                           \
    printf("cudaMallocAsync failed for var " #var " with size %zu\n", (size_t)size);   \
    printf("CUDA Runtime Error: %s\n",  cudaGetErrorString(status));          \
    assert(status == cudaSuccess);                                            \
  }                                                                           \
}                                                                             

#define destroyCUDA(var) \
{ \
  cudaError_t status = cudaFree(var); \
  if (status != cudaSuccess) \
  { \
    printf("cudaFree failed for " #var "\n"); \
    checkCUDA(status);                        \
    assert(status == cudaSuccess);            \
  } \
}

#define destroyAsyncCUDA(var, stream) \
{ \
  cudaError_t status = cudaFreeAsync(var, stream); \
  if (status != cudaSuccess) \
  { \
    printf("cudaFreeAsync failed for " #var "\n"); \
    assert(status == cudaSuccess);            \
  } \
}

#define allocateCopyCUDA(var, rhs, size) \
{\
    allocateCUDA(var, size, decltype(*rhs)); \
    checkCUDA(cudaMemcpy(var, rhs, sizeof(decltype(*rhs))*(size), cudaMemcpyHostToDevice)); \
}

#define memcpyD2HCUDA(var, rhs, size) \
{                                          \
  checkCUDA(cudaMemcpy(var, rhs, sizeof(decltype(*rhs)) * (size), cudaMemcpyDeviceToHost)); \
}

#define memcpyH2DCUDA(var, rhs, size) \
{                                          \
  checkCUDA(cudaMemcpy(var, rhs, sizeof(decltype(*rhs)) * (size), cudaMemcpyHostToDevice)); \
}

#define memcpyAsyncD2HCUDA(var, rhs, size, stream) \
{                                          \
  checkCUDA(cudaMemcpyAsync(var, rhs, sizeof(decltype(*rhs)) * (size), cudaMemcpyDeviceToHost, stream)); \
}

#define memcpyAsyncH2DCUDA(var, rhs, size, stream) \
{                                          \
  checkCUDA(cudaMemcpyAsync(var, rhs, sizeof(decltype(*rhs)) * (size), cudaMemcpyHostToDevice, stream)); \
}

#endif
