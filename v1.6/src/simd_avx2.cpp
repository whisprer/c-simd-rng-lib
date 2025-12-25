#include "avx2_impl.h"
#include "rng_core.h"
#include "rng_common.h"

#ifdef USE_AVX2
#include <immintrin.h>
#include <memory>
#include <array>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>
#include <cstring>

// Optimized AVX2 RNG State class with hybrid approach
class AVX2RNGState {
public:
    // 32-byte aligned for AVX2
    alignas(32) __m256i s0;
    alignas(32) __m256i s1;
    alignas(32) std::array<uint64_t, 4> results; // Reverted to std::array
    size_t next_idx; // Kept size_t instead of uint32_t for cache line alignment

    // Constructor with optimized initialization
    explicit AVX2RNGState(uint64_t seed) : next_idx(4) {
        // Initialize 4 parallel generators with different seeds
        alignas(32) std::array<uint64_t, 4> s0_vals, s1_vals;
        
        // Use SplitMix64 for better initialization
        uint64_t z = seed;
        for (int i = 0; i < 4; i++) {
            // First state variable
            z += 0x9e3779b97f4a7c15ULL;
            uint64_t x = z;
            x ^= x >> 30;
            x *= 0xbf58476d1ce4e5b9ULL;
            x ^= x >> 27;
            x *= 0x94d049bb133111ebULL;
            s0_vals[i] = x ^ (x >> 31);
            
            // Second state variable
            z += 0x9e3779b97f4a7c15ULL;
            x = z;
            x ^= x >> 30;
            x *= 0xbf58476d1ce4e5b9ULL;
            x ^= x >> 27;
            x *= 0x94d049bb133111ebULL;
            s1_vals[i] = x ^ (x >> 31);
        }
        
        // Load initial states into AVX2 registers (aligned load)
        s0 = _mm256_load_si256(reinterpret_cast<const __m256i*>(s0_vals.data()));
        s1 = _mm256_load_si256(reinterpret_cast<const __m256i*>(s1_vals.data()));
        
        // Pre-generate first batch of random numbers
        generate_batch();
    }
    
    // Generate a batch of 4 random numbers
    inline void generate_batch() {
        // Calculate new state and results using optimized AVX2 operations
        __m256i sum = _mm256_add_epi64(s0, s1);
        __m256i rotated_sum = rotl_avx2(sum, 17);
        __m256i result_vec = _mm256_add_epi64(rotated_sum, s0);
        
        // Store results using aligned store
        _mm256_store_si256(reinterpret_cast<__m256i*>(results.data()), result_vec);
        
        // Update state with optimized operations
        __m256i s1_xor_s0 = _mm256_xor_si256(s1, s0);
        s0 = _mm256_xor_si256(
                rotl_avx2(s0, 49),
                _mm256_xor_si256(
                    s1_xor_s0,
                    _mm256_slli_epi64(s1_xor_s0, 21)
                )
            );
        s1 = rotl_avx2(s1_xor_s0, 28);
        
        // Reset index
        next_idx = 0;
    }
    
    // Optimized rotation function for AVX2
    static inline __m256i rotl_avx2(__m256i x, int k) {
        return _mm256_or_si256(
            _mm256_slli_epi64(x, k),
            _mm256_srli_epi64(x, 64 - k)
        );
    }
};

// Simple but efficient version of next_u64 for single-number generation
uint64_t avx2_next_u64(void* state_ptr) {
    AVX2RNGState* state = static_cast<AVX2RNGState*>(state_ptr);
    
    // Check if we need to generate a new batch - no misleading branch prediction hints
    if (state->next_idx >= 4) {
        state->generate_batch();
    }
    
    // Return the next value
    return state->results[state->next_idx++];
}

// Fast direct double conversion
double avx2_next_double(void* state_ptr) {
    // Convert to double using high 53 bits
    uint64_t value = avx2_next_u64(state_ptr);
    return (value >> 11) * (1.0 / (1ULL << 53));
}

// Hybrid batch generation that balances simplicity with performance
void avx2_next_batch(void* state_ptr, uint64_t* results, size_t count) {
    AVX2RNGState* state = static_cast<AVX2RNGState*>(state_ptr);
    size_t generated = 0;
    
    // Fast path for batches that are multiples of 4
    if (count >= 4 && (count % 4) == 0) {
        // Process multiple batches of 4 values directly
        for (size_t i = 0; i < count; i += 4) {
            // Calculate result directly into results buffer
            __m256i s0 = state->s0;
            __m256i s1 = state->s1;
            
            __m256i sum = _mm256_add_epi64(s0, s1);
            __m256i rotated_sum = AVX2RNGState::rotl_avx2(sum, 17);
            __m256i result_vec = _mm256_add_epi64(rotated_sum, s0);
            
            // Store results directly to output buffer (potentially unaligned)
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(results + i), result_vec);
            
            // Update state
            __m256i s1_xor_s0 = _mm256_xor_si256(s1, s0);
            state->s0 = _mm256_xor_si256(
                        AVX2RNGState::rotl_avx2(s0, 49),
                        _mm256_xor_si256(
                            s1_xor_s0,
                            _mm256_slli_epi64(s1_xor_s0, 21)
                        )
                    );
            state->s1 = AVX2RNGState::rotl_avx2(s1_xor_s0, 28);
        }
        
        // Force next call to generate new batch
        state->next_idx = 4;
        return;
    }
    
    // General case for arbitrary counts
    
    // First, use any numbers already in the buffer
    if (state->next_idx < 4) {
        size_t available = 4 - state->next_idx;
        size_t to_copy = std::min(available, count);
        
        // Copy values from current batch
        std::copy_n(state->results.data() + state->next_idx, to_copy, results);
        
        generated += to_copy;
        state->next_idx += to_copy;
    }
    
    // Generate full batches of 4 directly into the results buffer
    while (generated + 4 <= count) {
        // Calculate result directly into results buffer
        __m256i s0 = state->s0;
        __m256i s1 = state->s1;
        
        __m256i sum = _mm256_add_epi64(s0, s1);
        __m256i rotated_sum = AVX2RNGState::rotl_avx2(sum, 17);
        __m256i result_vec = _mm256_add_epi64(rotated_sum, s0);
        
        // Store results directly to output buffer
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(results + generated), result_vec);
        
        // Update state
        __m256i s1_xor_s0 = _mm256_xor_si256(s1, s0);
        state->s0 = _mm256_xor_si256(
                    AVX2RNGState::rotl_avx2(s0, 49),
                    _mm256_xor_si256(
                        s1_xor_s0,
                        _mm256_slli_epi64(s1_xor_s0, 21)
                    )
                );
        state->s1 = AVX2RNGState::rotl_avx2(s1_xor_s0, 28);
        
        generated += 4;
    }
    
    // Generate final batch if needed for remaining values
    if (generated < count) {
        state->generate_batch();
        
        size_t remaining = count - generated;
        std::copy_n(state->results.data(), remaining, results + generated);
        state->next_idx = remaining;
    }
}

// Factory function
void* avx2_new(uint64_t seed) {
    try {
        return new AVX2RNGState(seed);
    } catch (const std::exception&) {
        return nullptr;
    }
}

// Cleanup function
void avx2_free(void* state) {
    delete static_cast<AVX2RNGState*>(state);
}

#endif // USE_AVX2