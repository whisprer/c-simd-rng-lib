#ifndef WYRAND_IMPL_H
#define WYRAND_IMPL_H

#include "rng_core.h"
#include <immintrin.h>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

namespace rng {
namespace wyrand {

// Scalar implementation of WyRand
class WyRandScalar : public RNGBase {
public:
    explicit WyRandScalar(uint64_t seed) : state_(seed) {}
    
    uint64_t next_u64() override {
        state_ += 0xa0761d6478bd642FULL;
        uint128_t t = (uint128_t)(state_) * ((state_) ^ 0xe7037ed1a0b428dbULL);
        return (uint64_t)(t >> 64) ^ (uint64_t)t;
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
        return "WyRand Scalar";
    }
    
public:
    uint64_t state_;
    // Use __uint128_t if available, otherwise define a custom uint128_t type
    #if defined(__SIZEOF_INT128__)
        using uint128_t = __uint128_t;
    #else
        struct uint128_t {
            uint64_t low, high;
            
            uint128_t(uint64_t v) : low(v), high(0) {}
            uint128_t(uint64_t h, uint64_t l) : low(l), high(h) {}
            
            operator uint64_t() const { return low; }
        };
        
        inline uint128_t operator*(uint128_t a, uint128_t b) {
            // Simplified 64x64->128 multiplication
            uint64_t a0 = a.low & 0xFFFFFFFF;
            uint64_t a1 = a.low >> 32;
            uint64_t b0 = b.low & 0xFFFFFFFF;
            uint64_t b1 = b.low >> 32;
            
            uint64_t r0 = a0 * b0;
            uint64_t r1 = a1 * b0 + (r0 >> 32);
            uint64_t r2 = a0 * b1 + (r1 & 0xFFFFFFFF);
            uint64_t r3 = a1 * b1 + (r1 >> 32) + (r2 >> 32);
            
            return uint128_t(r3, (r2 << 32) | (r0 & 0xFFFFFFFF));
        }
        
        inline uint128_t operator>>(uint128_t a, int shift) {
            if (shift == 0) return a;
            if (shift >= 64) return uint128_t(0, a.high >> (shift - 64));
            return uint128_t(a.high >> shift, (a.high << (64 - shift)) | (a.low >> shift));
        }
        
        inline uint128_t operator^(uint128_t a, uint128_t b) {
            return uint128_t(a.high ^ b.high, a.low ^ b.low);
        }
    #endif
};

#ifdef USE_SSE2
// SSE2 implementation of WyRand
class WyRandSSE2 {
public:
    explicit WyRandSSE2(uint64_t seed) {
        auto seeds = generate_seeds<2>(seed);
        state_ = _mm_set_epi64x(seeds[1], seeds[0]);
    }
    
    void generate_batch(uint64_t* dest, size_t count) {
        for (size_t i = 0; i < count; i += 2) {
            // Add constants
            __m128i add_const = _mm_set1_epi64x(0xa0761d6478bd642FULL);
            state_ = _mm_add_epi64(state_, add_const);
            
            // Extract state for multiplication (no direct SSE2 64-bit multiply)
            alignas(16) uint64_t s[2];
            _mm_store_si128((__m128i*)s, state_);
            
            // Compute WyRand for each lane
            for (int j = 0; j < 2 && i + j < count; j++) {
                uint64_t x = s[j] ^ 0xe7037ed1a0b428dbULL;
                WyRandScalar::uint128_t t = (WyRandScalar::uint128_t)s[j] * x;
                dest[i + j] = (uint64_t)(t >> 64) ^ (uint64_t)t;
            }
        }
    }
    
    std::string get_implementation_name() const {
        return "WyRand SSE2";
    }
    
private:
    __m128i state_;
};
#endif

#ifdef USE_AVX2
// AVX2 implementation of WyRand
class WyRandAVX2 {
public:
    explicit WyRandAVX2(uint64_t seed) {
        auto seeds = generate_seeds<4>(seed);
        state_ = _mm256_loadu_si256((__m256i*)seeds.data());
    }
    
    void generate_batch(uint64_t* dest, size_t count) {
        for (size_t i = 0; i < count; i += 4) {
            // Add constants
            __m256i add_const = _mm256_set1_epi64x(0xa0761d6478bd642FULL);
            state_ = _mm256_add_epi64(state_, add_const);
            
            // Extract state for multiplication (no direct AVX2 64-bit multiply)
            alignas(32) uint64_t s[4];
            _mm256_store_si256((__m256i*)s, state_);
            
            // Compute WyRand for each lane
            for (int j = 0; j < 4 && i + j < count; j++) {
                uint64_t x = s[j] ^ 0xe7037ed1a0b428dbULL;
                WyRandScalar::uint128_t t = (WyRandScalar::uint128_t)s[j] * x;
                dest[i + j] = (uint64_t)(t >> 64) ^ (uint64_t)t;
            }
        }
    }
    
    std::string get_implementation_name() const {
        return "WyRand AVX2";
    }
    
private:
    __m256i state_;
};
#endif

#ifdef USE_AVX512
// AVX512 implementation of WyRand
class WyRandAVX512 {
public:
    explicit WyRandAVX512(uint64_t seed) {
        auto seeds = generate_seeds<8>(seed);
        state_ = _mm512_loadu_si512((__m512i*)seeds.data());
    }
    
    void generate_batch(uint64_t* dest, size_t count) {
        for (size_t i = 0; i < count; i += 8) {
            // Add constants
            __m512i add_const = _mm512_set1_epi64(0xa0761d6478bd642FULL);
            state_ = _mm512_add_epi64(state_, add_const);
            
            // Extract state for multiplication
            alignas(64) uint64_t s[8];
            _mm512_store_si512((__m512i*)s, state_);
            
            // Compute WyRand for each lane
            for (int j = 0; j < 8 && i + j < count; j++) {
                uint64_t x = s[j] ^ 0xe7037ed1a0b428dbULL;
                WyRandScalar::uint128_t t = (WyRandScalar::uint128_t)s[j] * x;
                dest[i + j] = (uint64_t)(t >> 64) ^ (uint64_t)t;
            }
        }
    }
    
    std::string get_implementation_name() const {
        return "WyRand AVX512";
    }
    
private:
    __m512i state_;
};
#endif

#ifdef USE_NEON
// ARM NEON implementation of WyRand
class WyRandNeon {
public:
    explicit WyRandNeon(uint64_t seed) {
        auto seeds = generate_seeds<2>(seed);
        state_ = vld1q_u64(seeds.data());
    }
    
    void generate_batch(uint64_t* dest, size_t count) {
        for (size_t i = 0; i < count; i += 2) {
            // Add constants
            uint64x2_t add_const = vdupq_n_u64(0xa0761d6478bd642FULL);
            state_ = vaddq_u64(state_, add_const);
            
            // Extract state for multiplication (no direct NEON 64-bit multiply)
            alignas(16) uint64_t s[2];
            vst1q_u64(s, state_);
            
            // Compute WyRand for each lane
            for (int j = 0; j < 2 && i + j < count; j++) {
                uint64_t x = s[j] ^ 0xe7037ed1a0b428dbULL;
                WyRandScalar::uint128_t t = (WyRandScalar::uint128_t)s[j] * x;
                dest[i + j] = (uint64_t)(t >> 64) ^ (uint64_t)t;
            }
        }
    }
    
    std::string get_implementation_name() const {
        return "WyRand NEON";
    }
    
private:
    uint64x2_t state_;
};
#endif

// Factory for creating the appropriate WyRand implementation
class WyRandFactory : public RNGFactory {
public:
    std::unique_ptr<RNGBase> create(uint64_t seed) override {
        ImplType impl = detect_best_impl();
        
        switch (impl) {
        #ifdef USE_OPENCL
            case ImplType::OpenCL:
                return std::make_unique<BufferedRNG<WyRandScalar>>(seed); // Fallback for now
        #endif
        
        #ifdef USE_AVX512
            case ImplType::AVX512:
                return std::make_unique<BufferedRNG<WyRandAVX512, 8>>(seed);
        #endif
        
        #ifdef USE_AVX2
            case ImplType::AVX2:
                return std::make_unique<BufferedRNG<WyRandAVX2, 4>>(seed);
        #endif
        
        #ifdef USE_AVX
            case ImplType::AVX:
                return std::make_unique<BufferedRNG<WyRandScalar>>(seed); // Fallback
        #endif
        
        #ifdef USE_NEON
            case ImplType::NEON:
                return std::make_unique<BufferedRNG<WyRandNeon, 2>>(seed);
        #endif
        
        #ifdef USE_SSE2
            case ImplType::SSE2:
                return std::make_unique<BufferedRNG<WyRandSSE2, 2>>(seed);
        #endif
        
            case ImplType::Scalar:
            default:
                return std::make_unique<WyRandScalar>(seed);
        }
    }
};

} // namespace wyrand
} // namespace rng

#endif // WYRAND_IMPL_H