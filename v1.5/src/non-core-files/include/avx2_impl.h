#ifndef AVX2_IMPL_H
#define AVX2_IMPL_H

#include <cstdint>
#include <cstddef>
#include <memory>
#include <array>

#ifdef _WIN32
#include <immintrin.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <x86intrin.h>
#endif

// Function declarations for AVX2 implementation
void* avx2_new(uint64_t seed);
uint64_t avx2_next_u64(void* state);
double avx2_next_double(void* state);
void avx2_next_batch(void* state, uint64_t* results, size_t count);
void avx2_free(void* state);

#endif // AVX2_IMPL_H
