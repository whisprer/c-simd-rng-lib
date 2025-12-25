#include "avx512_impl.h"
#include "rng_core.h"
#include "rng_common.h"

#ifdef USE_AVX512
#include <immintrin.h>
#include <memory>
#include <array>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>
#include <cstring>

// Implementation of AVX512RNGState
AVX512RNGState::AVX512RNGState(uint64_t seed) : next_idx_(8) {
    initialize_state(seed);
}

// Static factory method
std::unique_ptr<AVX512RNGState> AVX512RNGState::create(uint64_t seed) {
    return std::make_unique<AVX512RNGState>(seed);
}

uint64_t AVX512RNGState::next_u64() {
    // If we've used all pre-generated numbers, generate a new batch
    if (next_idx_ >= 8) {
        generate_batch();
    }
    
    return results_[next_idx_++];
}

double AVX512RNGState::next_double() {
    // Convert to double using the high 53 bits
    return (next_u64() >> 11) * (1.0 / (1ULL << 53));
}

void AVX512RNGState::next_batch(uint64_t* results, size_t count) {
    size_t generated = 0;
    
    while (generated < count) {
        // How many values can we get from current batch
        size_t available = 8 - next_idx_;
        size_t to_copy = std::min(available, count - generated);
        
        // Copy values from current batch
        std::copy_n(results_.data() + next_idx_, to_copy, results + generated);
        
        generated += to_copy;
        next_idx_ += to_copy;
        
        // If we need more values, generate a new batch
        if (next_idx_ >= 8 && generated < count) {
            generate_batch();
            next_idx_ = 0;
        }
    }
}

const char* AVX512RNGState::get_implementation_name() const {
    return "AVX512";
}

void AVX512RNGState::generate_batch() {
    // Calculate new state and results
    __m512i s0 = s0_;
    __m512i s1 = s1_;
    
    // Calculate result: rotl(s0 + s1, 17) + s0
    __m512i sum = _mm512_add_epi64(s0, s1);
    __m512i rotated_sum = rotl_avx512(sum, 17);
    __m512i result_vec = _mm512_add_epi64(rotated_sum, s0);
    
    // Store results
    _mm512_storeu_si512((__m512i*)results_.data(), result_vec);
    
    // Update state
    s1 = _mm512_xor_si512(s1, s0);
    s0_ = _mm512_xor_si512(
            rotl_avx512(s0, 49),
            _mm512_xor_si512(
                s1,
                _mm512_slli_epi64(s1, 21)
            )
        );
    s1_ = rotl_avx512(s1, 28);
}

void AVX512RNGState::initialize_state(uint64_t seed) {
    // Initialize 8 parallel generators with different seeds
    std::array<uint64_t, 8> seeds;
    seeds[0] = seed;
    for (int i = 1; i < 8; i++) {
        seeds[i] = seed + i;
    }
    
    std::array<uint64_t, 8> s0_vals, s1_vals;
    
    for (int i = 0; i < 8; i++) {
        // Use SplitMix64 seeding for each stream
        uint64_t z = (seeds[i] + 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        s0_vals[i] = z ^ (z >> 31);
        
        z = (s0_vals[i] + 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        s1_vals[i] = z ^ (z >> 31);
    }
    
    // Load initial states into AVX-512 registers
    s0_ = _mm512_loadu_si512((__m512i*)s0_vals.data());
    s1_ = _mm512_loadu_si512((__m512i*)s1_vals.data());
    
    // Pre-generate first batch of random numbers
    _mm512_storeu_si512((__m512i*)results_.data(), 
        _mm512_add_epi64(
            rotl_avx512(_mm512_add_epi64(s0_, s1_), 17), 
            s0_
        )
    );
}

__m512i AVX512RNGState::rotl_avx512(__m512i x, int k) {
    return _mm512_or_si512(
        _mm512_slli_epi64(x, k),
        _mm512_srli_epi64(x, 64 - k)
    );
}

// C-compatible function implementations
extern "C" {
    void* avx512_new(uint64_t seed) {
        return AVX512RNGState::create(seed).release();
    }

    uint64_t avx512_next_u64(void* state) {
        return static_cast<AVX512RNGState*>(state)->next_u64();
    }

    double avx512_next_double(void* state) {
        return static_cast<AVX512RNGState*>(state)->next_double();
    }

    void avx512_next_batch(void* state, uint64_t* results, size_t count) {
        static_cast<AVX512RNGState*>(state)->next_batch(results, count);
    }

    void avx512_free(void* state) {
        delete static_cast<AVX512RNGState*>(state);
    }
}
#endif // USE_AVX512