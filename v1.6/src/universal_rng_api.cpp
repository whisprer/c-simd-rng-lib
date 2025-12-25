#include "rng_includes.h"
#include "universal_rng_types.h"
#include "cpu_detect.h"

// Include implementations for AVX2, SSE2, AVX512, and scalar
#include "avx2_impl.h"
#include "sse2_impl.h"
#include "avx512_impl.h"
#include "scalar_impl.h"

extern "C" {

universal_rng_t* universal_rng_new(uint64_t seed, int algorithm_type, int precision_mode) {
    std::cout << "Creating RNG...\n";
    
    // Create a new universal RNG
    universal_rng_t* rng = new universal_rng_t;
    
    // Initialize all fields
    rng->state = nullptr;
    rng->next_u64 = nullptr;
    rng->next_double = nullptr;
    rng->generate_batch = nullptr;
    rng->free_func = nullptr;
    rng->implementation_type = RNG_IMPL_SCALAR;
    rng->algorithm_type = static_cast<RNGAlgorithmType>(algorithm_type);
    rng->precision_mode = static_cast<RNGPrecisionMode>(precision_mode);
    
    // Detect CPU features
    bool has_avx512_support = detect_avx512_support();
    bool has_avx2_support = detect_avx2_support();
    bool has_sse2_support = detect_sse2_support();
    
    // Detect best available implementation
    void* state = nullptr;

    std::cout << "CPU feature detection:\n";
    std::cout << "  SSE2: " << (has_sse2_support ? "Yes" : "No") << "\n";
    std::cout << "  AVX2: " << (has_avx2_support ? "Yes" : "No") << "\n";
    std::cout << "  AVX512: " << (has_avx512_support ? "Yes" : "No") << "\n";
    
    // Try AVX512 first if available
    #ifdef USE_AVX512
    if (has_avx512_support) {
        std::cout << "Trying AVX512 implementation...\n";
        try {
            state = avx512_new(seed);
            if (state) {
                std::cout << "Using AVX512 implementation\n";
                rng->implementation_type = RNG_IMPL_AVX512;
                rng->next_u64 = avx512_next_u64;
                rng->next_double = avx512_next_double;
                rng->generate_batch = avx512_next_batch;
                rng->free_func = avx512_free;
            } else {
                std::cout << "AVX512 implementation created null state\n";
            }
        } catch (const std::exception& e) {
            std::cout << "Exception in AVX512 initialization: " << e.what() << "\n";
        } catch (...) {
            std::cout << "Unknown exception in AVX512 initialization\n";
        }
    } else {
        std::cout << "AVX512 not supported by CPU, skipping\n";
    }
    #else
    std::cout << "AVX512 not compiled in, skipping\n";
    #endif
    
    // If AVX512 failed or wasn't available, try AVX2
    if (!state) {
        #ifdef USE_AVX2
        if (has_avx2_support) {
            std::cout << "Trying AVX2 implementation...\n";
            try {
                state = avx2_new(seed);
                if (state) {
                    std::cout << "Using AVX2 implementation\n";
                    rng->implementation_type = RNG_IMPL_AVX2;
                    rng->next_u64 = avx2_next_u64;
                    rng->next_double = avx2_next_double;
                    rng->generate_batch = avx2_next_batch;
                    rng->free_func = avx2_free;
                } else {
                    std::cout << "AVX2 implementation created null state\n";
                }
            } catch (const std::exception& e) {
                std::cout << "Exception in AVX2 initialization: " << e.what() << "\n";
            } catch (...) {
                std::cout << "Unknown exception in AVX2 initialization\n";
            }
        } else {
            std::cout << "AVX2 not supported by CPU, skipping\n";
        }
        #else
        std::cout << "AVX2 not compiled in, skipping\n";
        #endif
    }
    
    // If AVX2 failed or wasn't available, try SSE2
    if (!state) {
        #ifdef USE_SSE2
        if (has_sse2_support) {
            std::cout << "Trying SSE2 implementation...\n";
            try {
                state = sse2_new(seed);
                if (state) {
                    std::cout << "Using SSE2 implementation\n";
                    rng->implementation_type = RNG_IMPL_SSE2;
                    rng->next_u64 = sse2_next_u64;
                    rng->next_double = sse2_next_double;
                    rng->generate_batch = sse2_next_batch;
                    rng->free_func = sse2_free;
                } else {
                    std::cout << "SSE2 implementation created null state\n";
                }
            } catch (const std::exception& e) {
                std::cout << "Exception in SSE2 initialization: " << e.what() << "\n";
            } catch (...) {
                std::cout << "Unknown exception in SSE2 initialization\n";
            }
        } else {
            std::cout << "SSE2 not supported by CPU, skipping\n";
        }
        #else
        std::cout << "SSE2 not compiled in, skipping\n";
        #endif
    }
    
    // If all else fails, fall back to scalar
    if (!state) {
        std::cout << "Using scalar implementation\n";
        try {
            state = scalar_new(seed);
            if (state) {
                rng->implementation_type = RNG_IMPL_SCALAR;
                rng->next_u64 = scalar_next_u64;
                rng->next_double = scalar_next_double;
                rng->generate_batch = scalar_next_batch;
                rng->free_func = scalar_free;
            } else {
                std::cout << "ERROR: Scalar implementation created null state\n";
                delete rng;
                return nullptr;
            }
        } catch (const std::exception& e) {
            std::cout << "Exception in scalar initialization: " << e.what() << "\n";
            delete rng;
            return nullptr;
        } catch (...) {
            std::cout << "Unknown exception in scalar initialization\n";
            delete rng;
            return nullptr;
        }
    }
    
    // Set the state
    rng->state = state;
    std::cout << "RNG created successfully\n";
    return rng;
}

// The rest of your API functions remain the same...
uint64_t universal_rng_next_u64(universal_rng_t* rng) {
    if (rng && rng->next_u64 && rng->state) {
        return rng->next_u64(rng->state);
    }
    return 0;
}

double universal_rng_next_double(universal_rng_t* rng) {
    if (rng && rng->next_double && rng->state) {
        return rng->next_double(rng->state);
    }
    return 0.0;
}

void universal_rng_generate_batch(universal_rng_t* rng, uint64_t* results, size_t count) {
    if (rng && rng->generate_batch && rng->state && results) {
        rng->generate_batch(rng->state, results, count);
    }
}

const char* universal_rng_get_implementation(universal_rng_t* rng) {
    if (!rng) return "Invalid RNG";
    
    std::string impl_name;
    
    // Get implementation name
    switch (rng->implementation_type) {
        case RNG_IMPL_SCALAR:
            impl_name = "Scalar";
            break;
        case RNG_IMPL_SSE2:
            impl_name = "SSE2";
            break;
        case RNG_IMPL_AVX2:
            impl_name = "AVX2";
            break;
        case RNG_IMPL_AVX512:
            impl_name = "AVX-512";
            break;
        default:
            impl_name = "Unknown";
            break;
    }
    
    // Get algorithm name
    switch (rng->algorithm_type) {
        case RNG_ALGORITHM_XOROSHIRO:
            impl_name += " Xoroshiro128++";
            break;
        case RNG_ALGORITHM_WYRAND:
            impl_name += " WyRand";
            break;
        default:
            impl_name += " Unknown Algorithm";
            break;
    }
    
    // Allocate and return string
    char* result = new char[impl_name.length() + 1];
    strcpy(result, impl_name.c_str());
    return result;
}

void universal_rng_free_string(const char* str) {
    delete[] str;
}

void universal_rng_free(universal_rng_t* rng) {
    if (rng) {
        if (rng->free_func && rng->state) {
            rng->free_func(rng->state);
        }
        delete rng;
    }
}

}  // extern "C"