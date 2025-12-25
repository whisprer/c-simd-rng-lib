#ifndef UNIVERSAL_RNG_H
#define UNIVERSAL_RNG_H

#include "universal_rng_export.h"  // Add this include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

// Explicit opaque struct declaration
typedef struct universal_rng_t universal_rng_t;

// Add UNIVERSAL_RNG_API to all function declarations
UNIVERSAL_RNG_API universal_rng_t* universal_rng_new(uint64_t seed, int algorithm_type, int precision_mode);
UNIVERSAL_RNG_API uint64_t universal_rng_next_u64(universal_rng_t* rng);
UNIVERSAL_RNG_API double universal_rng_next_double(universal_rng_t* rng);
UNIVERSAL_RNG_API void universal_rng_generate_batch(universal_rng_t* rng, uint64_t* results, size_t count);
UNIVERSAL_RNG_API const char* universal_rng_get_implementation(universal_rng_t* rng);
UNIVERSAL_RNG_API void universal_rng_free_string(const char* str);
UNIVERSAL_RNG_API void universal_rng_free(universal_rng_t* rng);

// Extended bit-width functions
UNIVERSAL_RNG_API uint16_t universal_rng_next_u16(universal_rng_t* rng);
UNIVERSAL_RNG_API uint32_t universal_rng_next_u32(universal_rng_t* rng);

// For larger numbers, we'll need output buffers
UNIVERSAL_RNG_API void universal_rng_next_u128(universal_rng_t* rng, uint64_t* output); // 2 x uint64_t   
UNIVERSAL_RNG_API void universal_rng_next_u256(universal_rng_t* rng, uint64_t* output); // 4 × uint64_t
UNIVERSAL_RNG_API void universal_rng_next_u512(universal_rng_t* rng, uint64_t* output); // 8 × uint64_t
UNIVERSAL_RNG_API void universal_rng_next_u1024(universal_rng_t* rng, uint64_t* output); // 16 × uint64_t

// Batch generation for different bit widths
UNIVERSAL_RNG_API void universal_rng_generate_batch_u16(universal_rng_t* rng, uint16_t* results, size_t count);
UNIVERSAL_RNG_API void universal_rng_generate_batch_u32(universal_rng_t* rng, uint32_t* results, size_t count);

// Batch for uint64_t already exists
UNIVERSAL_RNG_API void universal_rng_generate_batch_u128(universal_rng_t* rng, uint64_t* results, size_t count);
UNIVERSAL_RNG_API void universal_rng_generate_batch_u256(universal_rng_t* rng, uint64_t* results, size_t count);
UNIVERSAL_RNG_API void universal_rng_generate_batch_u512(universal_rng_t* rng, uint64_t* results, size_t count);
UNIVERSAL_RNG_API void universal_rng_generate_batch_u1024(universal_rng_t* rng, uint64_t* results, size_t count);

#ifdef __cplusplus
}
#endif

#endif // UNIVERSAL_RNG_H