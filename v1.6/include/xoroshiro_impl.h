#ifndef XOROSHIRO_IMPL_H
#define XOROSHIRO_IMPL_H

#include "rng_core.h"
#include <immintrin.h>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

namespace rng {
namespace xoroshiro {

// Scalar implementation of xoroshiro128++
class Xoroshiro128ppScalar : public RNGBase {
public:
    explicit Xoroshiro128ppScalar(uint64_t seed) {
        seed_state(seed);
    }
    
    uint64_t next_u64() override {
        const uint64_t s0 = state_[0];
        uint64_t s1 = state_[1];
        const uint64_t result = rotl64(s0 + s1, 17) + s0;
        
        s1 ^= s0;
        state_[0] = rotl64(s0, 49) ^ s1 ^ (s1 << 21);
        state_[1] = rotl64(s1, 28);
        
        return result;
    }
    
    double next_double() override {
        return to_double(next_u64());
    }
    
    void generate_batch(uint64_t* dest, size_t count) override {
        for (size_t i = 0; i < count; i++) {
            dest[i] = next_u64();
        }
    }
    
    std::string get_implementation_name() const override {
        return "Xoroshiro128++ Scalar";
    }
    
    // Jump ahead by 2^64 calls to next_u64()
    void jump() {
        static const std::array<uint64_t, 2> JUMP = { 0xdf900294d8f554a5, 0x170865df4b3201fc };
        
        std::array<uint64_t, 2> s = {0, 0};
        for(const auto& jump_val : JUMP) {
            for(int b = 0; b < 64; b++) {
                if (jump_val & (1ULL << b)) {
                    s[0] ^= state_[0];
                    s[1] ^= state_[1];
                }
                next_u64();
            }
        }
        
        state_ = s;
    }
    
private:
    std::array<uint64_t, 2> state_;
    
    // Initialize state from a 64-bit seed using SplitMix64
    void seed_state(uint64_t seed) {
        uint64_t z = (seed + 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        state_[0] = z ^ (z >> 31);

        z = (state_[0] + 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        state_[1] = z ^ (z >> 31);
    }
};

// Helper for AVX2 rotl
static inline __m256i rotl_avx2(__m256i x, int k) {
    return _mm256_or_si256(_mm256_slli_epi64(x, k), _mm256_srli_epi64(x, 64 - k));
}

// Helper for AVX-512 rotl
static inline __m512i rotl_avx512(__m512i x, int k) {
    return _mm512_or_si512(_mm512_slli_epi64(x, k), _mm512_srli_epi64(x, 64 - k));
}

// Helper for NEON rotl
#ifdef USE_NEON
static inline uint64x2_t rotl_neon(uint64x2_t x, int k) {
    return vsriq_n_u64(vshlq_n_u64(x, k), x, 64 - k);
}
#endif

#ifdef USE_SSE2
// SSE2 implementation of xoroshiro128++
class Xoroshiro128ppSSE2 {
public:
    explicit Xoroshiro128ppSSE2(uint64_t seed) {
        auto seeds = generate_seeds<4>(seed);
        state0_ = _mm_set_epi64x(seeds[1], seeds[0]);
        state1_ = _mm_set_epi64x(seeds[3], seeds[2]);
    }
    
    void generate_batch(uint64_t* dest, size_t count) {
        size_t i = 0;
        
        // Generate 2 numbers at a time
        for (; i + 1 < count; i += 2) {
            alignas(16) uint64_t s0[2], s1[2];
            _mm_store_si128((__m128i*)s0, state0_);
            _mm_store_si128((__m128i*)s1, state1_);
            
            // No direct 64-bit rotl in SSE2, do manually per-value
            for (int j = 0; j < 2; j++) {
                // Calculate result
                uint64_t sum = s0[j] + s1[j];
                uint64_t rotated_sum = rotl64(sum, 17);
                dest[i + j] = rotated_sum + s0[j];
                
                // Update state
                uint64_t s1_xor_s0 = s1[j] ^ s0[j];
                s0[j] = rotl64(s0[j], 49) ^ s1_xor_s0 ^ (s1_xor_s0 << 21);
                s1[j] = rotl64(s1_xor_s0, 28);
            }
            
            // Load updated state back
            state0_ = _mm_load_si128((__m128i*)s0);
            state1_ = _mm_load_si128((__m128i*)s1);
        }
        
        // Handle remaining values
        if (i < count) {
            alignas(16) uint64_t s0[2], s1[2];
            _mm_store_si128((__m128i*)s0, state0_);
            _mm_store_si128((__m128i*)s1, state1_);
            
            // Calculate result for the last value
            uint64_t sum = s0[0] + s1[0];
            uint64_t rotated_sum = rotl64(sum, 17);
            dest[i] = rotated_sum + s0[0];
            
            // Update state
            uint64_t s1_xor_s0 = s1[0] ^ s0[0];
            s0[0] = rotl64(s0[0], 49) ^ s1_xor_s0 ^ (s1_xor_s0 << 21);
            s1[0] = rotl64(s1_xor_s0, 28);
            
            // Load updated state back
            state0_ = _mm_load_si128((__m128i*)s0);
            state1_ = _mm_load_si128((__m128i*)s1);
        }
    }
    
    std::string get_implementation_name() const {
        return "Xoroshiro128++ SSE2";
    }
    
private:
    __m128i state0_, state1_;
};
#endif

#ifdef USE_AVX2
// AVX2 implementation of xoroshiro128++
class Xoroshiro128ppAVX2 {
public:
    explicit Xoroshiro128ppAVX2(uint64_t seed) {
        auto seeds = generate_seeds<8>(seed);
        state0_ = _mm256_loadu_si256((__m256i*)seeds.data());
        state1_ = _mm256_loadu_si256((__m256i*)(seeds.data() + 4));
    }
    
    void generate_batch(uint64_t* dest, size_t count) {
        size_t i = 0;
        
        // Generate 4 numbers at a time
        for (; i + 3 < count; i += 4) {
            // Calculate output - scrambler is: rotl(s0 + s1, 17) + s0
            __m256i sum = _mm256_add_epi64(state0_, state1_);
            __m256i rotated_sum = rotl_avx2(sum, 17);
            __m256i result = _mm256_add_epi64(rotated_sum, state0_);
            
            // Store results
            _mm256_storeu_si256((__m256i*)(dest + i), result);
            
            // Update state
            __m256i s1 = _mm256_xor_si256(state0_, state1_);
            
            state0_ = _mm256_xor_si256(
                          rotl_avx2(state0_, 49),
                          _mm256_xor_si256(
                              s1,
                              _mm256_slli_epi64(s1, 21)
                          )
                        );
                        
            state1_ = rotl_avx2(s1, 28);
        }

        // Handle remaining values
        if (i < count) {
            alignas(32) uint64_t s0[4], s1[4], results[4];
            _mm256_store_si256((__m256i*)s0, state0_);
            _mm256_store_si256((__m256i*)s1, state1_);
            
            // Calculate results for remaining values
            for (size_t j = 0; j < count - i; j++) {
                uint64_t sum = s0[j] + s1[j];
                uint64_t rotated_sum = rotl64(sum, 17);
                results[j] = rotated_sum + s0[j];
                
                // Update state
                uint64_t s1_xor_s0 = s1[j] ^ s0[j];
                s0[j] = rotl64(s0[j], 49) ^ s1_xor_s0 ^ (s1_xor_s0 << 21);
                s1[j] = rotl64(s1_xor_s0, 28);
            }
            
            // Copy results
            std::memcpy(dest + i, results, (count - i) * sizeof(uint64_t));
            
            // Load updated state back
            state0_ = _mm256_load_si256((__m256i*)s0);
            state1_ = _mm256_load_si256((__m256i*)s1);
        }
    }
    
    std::string get_implementation_name() const {
        return "Xoroshiro128++ AVX2";
    }
    
private:
    __m256i state0_, state1_;
};
#endif

#ifdef USE_AVX512
// AVX512 implementation of xoroshiro128++
class Xoroshiro128ppAVX512 {
public:
    explicit Xoroshiro128ppAVX512(uint64_t seed) {
        auto seeds = generate_seeds<16>(seed);
        state0_ = _mm512_loadu_si512((__m512i*)seeds.data());
        state1_ = _mm512_loadu_si512((__m512i*)(seeds.data() + 8));
    }
    
    void generate_batch(uint64_t* dest, size_t count) {
        size_t i = 0;
        
        // Generate 8 numbers at a time
        for (; i + 7 < count; i += 8) {
            // Calculate output
            __m512i sum = _mm512_add_epi64(state0_, state1_);
            __m512i rotated_sum = rotl_avx512(sum, 17);
            __m512i result = _mm512_add_epi64(rotated_sum, state0_);
            
            // Store results
            _mm512_storeu_si512((__m512i*)(dest + i), result);
            
            // Update state
            __m512i s1 = _mm512_xor_si512(state0_, state1_);
            
            state0_ = _mm512_xor_si512(
                          rotl_avx512(state0_, 49),
                          _mm512_xor_si512(
                              s1,
                              _mm512_slli_epi64(s1, 21)
                          )
                        );
                        
            state1_ = rotl_avx512(s1, 28);
        }
        
        // Handle remaining values
        if (i < count) {
            alignas(64) uint64_t s0[8], s1[8], results[8];
            _mm512_store_si512((__m512i*)s0, state0_);
            _mm512_store_si512((__m512i*)s1, state1_);
            
            // Calculate results for remaining values
            for (size_t j = 0; j < count - i; j++) {
                uint64_t sum = s0[j] + s1[j];
                uint64_t rotated_sum = rotl64(sum, 17);
                results[j] = rotated_sum + s0[j];
                
                // Update state
                uint64_t s1_xor_s0 = s1[j] ^ s0[j];
                s0[j] = rotl64(s0[j], 49) ^ s1_xor_s0 ^ (s1_xor_s0 << 21);
                s1[j] = rotl64(s1_xor_s0, 28);
            }
            
            // Copy results
            std::memcpy(dest + i, results, (count - i) * sizeof(uint64_t));
            
            // Load updated state back
            state0_ = _mm512_load_si512((__m512i*)s0);
            state1_ = _mm512_load_si512((__m512i*)s1);
        }
    }
    
    std::string get_implementation_name() const {
        return "Xoroshiro128++ AVX512";
    }
    
private:
    __m512i state0_, state1_;
};
#endif

#ifdef USE_NEON
// NEON implementation of xoroshiro128++
class Xoroshiro128ppNeon {
public:
    explicit Xoroshiro128ppNeon(uint64_t seed) {
        auto seeds = generate_seeds<4>(seed);
        state0_ = vld1q_u64(seeds.data());
        state1_ = vld1q_u64(seeds.data() + 2);
    }
    
    void generate_batch(uint64_t* dest, size_t count) {
        size_t i = 0;
        
        // Generate 2 numbers at a time
        for (; i + 1 < count; i += 2) {
            // Calculate output
            uint64x2_t sum = vaddq_u64(state0_, state1_);
            uint64x2_t rotated_sum = rotl_neon(sum, 17);
            uint64x2_t result = vaddq_u64(rotated_sum, state0_);
            
            // Store results
            vst1q_u64(dest + i, result);
            
            // Update state
            uint64x2_t s1 = veorq_u64(state0_, state1_);
            
            state0_ = veorq_u64(
                        rotl_neon(state0_, 49),
                        veorq_u64(
                            s1,
                            vshlq_n_u64(s1, 21)
                        )
                      );
                      
            state1_ = rotl_neon(s1, 28);
        }
        
        // Handle remaining values
        if (i < count) {
            alignas(16) uint64_t s0[2], s1[2], results[2];
            vst1q_u64(s0, state0_);
            vst1q_u64(s1, state1_);
            
            // Calculate result for remaining value
            uint64_t sum = s0[0] + s1[0];
            uint64_t rotated_sum = rotl64(sum, 17);
            results[0] = rotated_sum + s0[0];
            
            // Update state
            uint64_t s1_xor_s0 = s1[0] ^ s0[0];
            s0[0] = rotl64(s0[0], 49) ^ s1_xor_s0 ^ (s1_xor_s0 << 21);
            s1[0] = rotl64(s1_xor_s0, 28);
            
            // Copy result
            dest[i] = results[0];
            
            // Load updated state back
            state0_ = vld1q_u64(s0);
            state1_ = vld1q_u64(s1);
        }
    }
    
    std::string get_implementation_name() const {
        return "Xoroshiro128++ NEON";
    }
    
private:
    uint64x2_t state0_, state1_;
};
#endif

// Factory for creating the appropriate xoroshiro128++ implementation
class Xoroshiro128ppFactory : public RNGFactory {
public:
    std::unique_ptr<RNGBase> create(uint64_t seed) override {
        ImplType impl = detect_best_impl();
        
        switch (impl) {
        #ifdef USE_OPENCL
            case ImplType::OpenCL:
                return std::make_unique<BufferedRNG<Xoroshiro128ppScalar>>(seed); // Fallback for now
        #endif
        
        #ifdef USE_AVX512
            case ImplType::AVX512:
                return std::make_unique<BufferedRNG<Xoroshiro128ppAVX512, 8>>(seed);
        #endif
        
        #ifdef USE_AVX2
            case ImplType::AVX2:
                return std::make_unique<BufferedRNG<Xoroshiro128ppAVX2, 4>>(seed);
        #endif
        
        #ifdef USE_AVX
            case ImplType::AVX:
                return std::make_unique<BufferedRNG<Xoroshiro128ppScalar>>(seed); // Fallback
        #endif
        
        #ifdef USE_NEON
            case ImplType::NEON:
                return std::make_unique<BufferedRNG<Xoroshiro128ppNeon, 2>>(seed);
        #endif
        
        #ifdef USE_SSE2
            case ImplType::SSE2:
                return std::make_unique<BufferedRNG<Xoroshiro128ppSSE2, 2>>(seed);
        #endif
        
            case ImplType::Scalar:
            default:
                return std::make_unique<Xoroshiro128ppScalar>(seed);
        }
    }
};

} // namespace xoroshiro
} // namespace rng

#endif // XOROSHIRO_IMPL_H