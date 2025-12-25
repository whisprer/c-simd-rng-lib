#ifndef RNG_CORE_H
#define RNG_CORE_H

#include <cstdint>
#include <memory>
#include <random>
#include <array>
#include <string>
#include <vector>

// Centralized SIMD detection macros with guards
#ifndef SIMD_FEATURES_DEFINED
#define SIMD_FEATURES_DEFINED

#if defined(__AVX512F__) && defined(__AVX512DQ__)
  #ifndef USE_AVX512
    #define USE_AVX512
  #endif
#elif defined(__AVX2__)
  #ifndef USE_AVX2
    #define USE_AVX2
  #endif
#elif defined(__AVX__)
  #ifndef USE_AVX
    #define USE_AVX
  #endif
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
  #ifndef USE_NEON
    #define USE_NEON
    #include <arm_neon.h>
  #endif
#elif defined(__SSE2__)
  #ifndef USE_SSE2
    #define USE_SSE2
  #endif
#endif

#endif // SIMD_FEATURES_DEFINED

// Set the parallel streams based on instruction set
#if defined(USE_OPENCL)
  #define RNG_PARALLEL_STREAMS 1024
#elif defined(USE_AVX512)
  #define RNG_PARALLEL_STREAMS 8
#elif defined(USE_AVX2) || defined(USE_AVX)
  #define RNG_PARALLEL_STREAMS 4
#elif defined(USE_NEON) || defined(USE_SSE2)
  #define RNG_PARALLEL_STREAMS 2
#else
  #define RNG_PARALLEL_STREAMS 1
#endif

namespace rng {

// Utility function
inline uint64_t rotl64(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

// Core RNG interface (fully virtual for polymorphic use)
class RNGBase {
public:
    virtual ~RNGBase() = default;

    virtual uint64_t next_u64() = 0;
    virtual double next_double() = 0;

    // Batch generation (default naive implementation)
    virtual void generate_batch(uint64_t* dest, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            dest[i] = next_u64();
        }
    }

    virtual std::string get_implementation_name() const {
        return "Unknown";
    }

protected:
    static inline double to_double(uint64_t value) {
        return (value >> 11) * (1.0 / (1ULL << 53));
    }
};

// Implementation types
enum class ImplType {
    Scalar,
    SSE2,
    AVX,
    AVX2,
    AVX512,
    NEON,
    OpenCL
};

// Utility function to convert ImplType to string
inline std::string impl_type_to_string(ImplType type) {
    switch (type) {
        case ImplType::Scalar: return "Scalar";
        case ImplType::SSE2: return "SSE2";
        case ImplType::AVX: return "AVX";
        case ImplType::AVX2: return "AVX2";
        case ImplType::AVX512: return "AVX512";
        case ImplType::NEON: return "NEON";
        case ImplType::OpenCL: return "OpenCL";
        default: return "Unknown";
    }
}

// Universal seed generator
template<size_t N>
inline std::array<uint64_t, N> generate_seeds(uint64_t seed) {
    std::array<uint64_t, N> seeds;
    seeds[0] = seed ? seed : 0x1234567890ABCDEFULL;

    for (size_t i = 1; i < N; i++) {
        uint64_t z = seeds[i - 1] + 0x9e3779b97f4a7c15ULL;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        seeds[i] = z ^ (z >> 31);
    }
    return seeds;
}

// RNG Factory Base Class
class RNGFactory {
    public:
        virtual ~RNGFactory() = default;
    
        virtual std::unique_ptr<RNGBase> create(uint64_t seed) = 0;
    
        // Detect best available implementation
        static ImplType detect_best_impl() {
            #if defined(USE_OPENCL)
                return ImplType::OpenCL;
            #elif defined(USE_AVX512)
                return ImplType::AVX512;
            #elif defined(USE_AVX2)
                return ImplType::AVX2;
            #elif defined(USE_AVX)
                return ImplType::AVX;
            #elif defined(USE_NEON)
                return ImplType::NEON;
            #elif defined(USE_SSE2)
                return ImplType::SSE2;
            #else
                return ImplType::Scalar;
            #endif
        }

};

// Buffered SIMD RNG wrapper
template<typename Impl, size_t BufferSize = RNG_PARALLEL_STREAMS>
class BufferedRNG : public RNGBase {
public:
    explicit BufferedRNG(uint64_t seed)
        : impl_(seed), buffer_pos_(BufferSize) {}

    uint64_t next_u64() override {
        if (buffer_pos_ >= BufferSize) {
            refill_buffer();
            buffer_pos_ = 0;
        }
        return buffer_[buffer_pos_++];
    }

    double next_double() override {
        return to_double(next_u64());
    }

    void generate_batch(uint64_t* dest, size_t count) override {
        size_t pos = 0;

        // Use remaining buffer values
        while (pos < count && buffer_pos_ < BufferSize) {
            dest[pos++] = buffer_[buffer_pos_++];
        }

        // Directly generate large batches
        if ((count - pos) >= BufferSize) {
            size_t direct_count = (count - pos) / BufferSize * BufferSize;
            impl_.generate_batch(dest + pos, direct_count);
            pos += direct_count;
        }

        // Fill buffer for remaining values
        if (pos < count) {
            refill_buffer();
            buffer_pos_ = 0;

            while (pos < count) {
                dest[pos++] = buffer_[buffer_pos_++];
            }
        }
    }

    std::string get_implementation_name() const override {
        return impl_.get_implementation_name();
    }

private:
    Impl impl_;
    std::array<uint64_t, BufferSize> buffer_;
    size_t buffer_pos_ = 0;

    void refill_buffer() {
        impl_.generate_batch(buffer_.data(), BufferSize);
    }
};

} // namespace rng

#endif // RNG_CORE_H
