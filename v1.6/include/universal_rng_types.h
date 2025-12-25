#ifndef UNIVERSAL_RNG_TYPES_H
#define UNIVERSAL_RNG_TYPES_H

#include <cstdint>
#include <cstddef>

// Prevent multiple definitions of USE_AVX512
#undef USE_AVX512
#undef USE_AVX2
#undef USE_SSE2

// Detect SIMD support
#if defined(__AVX512F__) && defined(__AVX512DQ__)
    #define USE_AVX512
#elif defined(__AVX2__)
    #define USE_AVX2
#elif defined(__SSE2__)
    #define USE_SSE2
#endif

// Enum for RNG Algorithm Types
enum RNGAlgorithmType {
    RNG_ALGORITHM_XOROSHIRO = 0,
    RNG_ALGORITHM_WYRAND = 1
};

// Enum for Precision Modes
enum RNGPrecisionMode {
    RNG_PRECISION_SINGLE = 0,
    RNG_PRECISION_DOUBLE = 1
};

// Enum for Implementation Types
enum RngImplType {
    RNG_IMPL_SCALAR = 0,
    RNG_IMPL_SSE2 = 1,
    RNG_IMPL_AVX = 2,
    RNG_IMPL_AVX2 = 3,
    RNG_IMPL_AVX512 = 4,
    RNG_IMPL_NEON = 5,
    RNG_IMPL_OPENCL = 6
};

// Complete definition of the universal_rng_t struct (not just a forward declaration)
struct universal_rng_t {
    void* state;                                     // Actual RNG state
    uint64_t (*next_u64)(void*);                     // Function to generate next 64-bit random number
    double (*next_double)(void*);                    // Function to generate next double
    void (*generate_batch)(void*, uint64_t*, size_t); // Batch generation
    void (*free_func)(void*);                        // Function to free the state
    RngImplType implementation_type;                 // Implementation type (scalar, SSE2, etc.)
    RNGAlgorithmType algorithm_type;                 // Algorithm type (Xoroshiro, WyRand)
    RNGPrecisionMode precision_mode;                 // Precision mode
};

// C-style function declarations for Universal RNG
#ifdef __cplusplus
extern "C" {
#endif

universal_rng_t* universal_rng_new(uint64_t seed, int algorithm_type, int precision_mode);
uint64_t universal_rng_next_u64(universal_rng_t* rng);
double universal_rng_next_double(universal_rng_t* rng);
void universal_rng_generate_batch(universal_rng_t* rng, uint64_t* results, size_t count);
const char* universal_rng_get_implementation(universal_rng_t* rng);
void universal_rng_free(universal_rng_t* rng);

#ifdef __cplusplus
}
#endif

#endif // UNIVERSAL_RNG_TYPES_H