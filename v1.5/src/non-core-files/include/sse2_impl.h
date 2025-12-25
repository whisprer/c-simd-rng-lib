#pragma once
#include <cstdint>
#include <cstddef>

/*  Public C API – visible from every other translation unit  */
#ifdef __cplusplus
extern "C" {
#endif

void*     sse2_new(uint64_t seed);
uint64_t  sse2_next_u64(void* state);
double    sse2_next_double(void* state);
void      sse2_next_batch(void* state, uint64_t* results, size_t count);
void      sse2_free(void* state);

#ifdef __cplusplus
}
#endif
