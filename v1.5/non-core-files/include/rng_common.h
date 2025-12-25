#ifndef RNG_COMMON_H
#define RNG_COMMON_H

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <cstdlib>
#include <new>  // For std::align_val_t

#if defined(_MSC_VER)
    #include <malloc.h>  // For _aligned_malloc
#endif

namespace rng {

// Aligned memory allocation RAII wrapper
template<typename T>
class AlignedBuffer {
public:
    AlignedBuffer(size_t count, size_t alignment = 64) {
        #if defined(_MSC_VER)
            m_data = reinterpret_cast<T*>(_aligned_malloc(count * sizeof(T), alignment));
            if (!m_data) {
                throw std::bad_alloc();
            }
        #else
            // For GCC, Clang, etc.
            // Alternative approach without posix_memalign
            size_t size = count * sizeof(T);
            void* ptr = nullptr;
            
            // Allocate extra space for alignment and pointer storage
            char* raw = new char[size + alignment + sizeof(void*)];
            if (!raw) {
                throw std::bad_alloc();
            }
            
            // Calculate aligned address
            char* aligned = raw + sizeof(void*);
            aligned += alignment - (reinterpret_cast<uintptr_t>(aligned) & (alignment - 1));
            
            // Store original pointer for deletion
            *(reinterpret_cast<char**>(aligned) - 1) = raw;
            
            m_data = reinterpret_cast<T*>(aligned);
        #endif
    }
    
    ~AlignedBuffer() {
        #if defined(_MSC_VER)
            _aligned_free(m_data);
        #else
            // Retrieve original pointer and delete
            if (m_data) {
                char* raw = *(reinterpret_cast<char**>(m_data) - 1);
                delete[] raw;
            }
        #endif
    }

    T* data() { return m_data; }

private:
    T* m_data;
};

// Generic benchmarking template function
template<typename RngType, typename GenFunc>
double benchmark_generator(uint64_t iterations, RngType& rng, GenFunc generator) {
    auto start = std::chrono::high_resolution_clock::now();
    
    uint64_t sum = 0;
    for(uint64_t i = 0; i < iterations; i++) {
        sum ^= generator(rng);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    
    // Anti-optimization trick
    if(sum == 1) std::cout << "Won't happen: " << sum << std::endl;
    
    return duration.count();
}

// SIMD detection
constexpr bool has_avx512() {
#ifdef USE_AVX512
    return true;
#else
    return false;
#endif
}

constexpr bool has_avx2() {
#ifdef USE_AVX2
    return true;
#else
    return false;
#endif
}

constexpr bool has_avx() {
#ifdef USE_AVX
    return true;
#else
    return false;
#endif
}

constexpr bool has_sse2() {
#ifdef USE_SSE2
    return true;
#else
    return false;
#endif
}

constexpr bool has_neon() {
#ifdef USE_NEON
    return true;
#else
    return false;
#endif
}

constexpr bool has_opencl() {
#ifdef USE_OPENCL
    return true;
#else
    return false;
#endif
}

// Helper to print SIMD support
inline void print_simd_support() {
    if (has_opencl()) {
        std::cout << "Detected GPU (OpenCL)" << std::endl;
    } else if (has_avx512()) {
        std::cout << "Detected AVX-512" << std::endl;
    } else if (has_avx2()) {
        std::cout << "Detected AVX2" << std::endl;
    } else if (has_avx()) {
        std::cout << "Detected AVX" << std::endl;
    } else if (has_neon()) {
        std::cout << "Detected NEON" << std::endl;
    } else if (has_sse2()) {
        std::cout << "Detected SSE2" << std::endl;
    } else {
        std::cout << "No SIMD (scalar fallback)" << std::endl;
    }
}


} // namespace rng

#endif // RNG_COMMON_H