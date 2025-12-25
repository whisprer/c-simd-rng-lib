#ifndef OPENCL_RNG_INTEGRATION_H
#define OPENCL_RNG_INTEGRATION_H

#include "universal_rng.h"
#include "opencl_rng_kernels.h"
#include "logging.h"
#include <memory>
#include <stdexcept>

// OpenCL RNG Integration Class
class OpenCLRNGIntegration {
public:
    // Create OpenCL RNG with smart pointer management
    static std::unique_ptr<universal_rng_t> createUniversalRNG(
        uint64_t seed, 
        RNGAlgorithmType algorithm,
        RNGPrecisionMode precision
    ) {
        // Validate inputs
        if (algorithm != RNG_ALGORITHM_XOROSHIRO && algorithm != RNG_ALGORITHM_WYRAND) {
            Logger::getInstance().log(Logger::Level::ERROR, 
                "Invalid RNG algorithm for OpenCL integration");
            return nullptr;
        }

        // Attempt to create OpenCL RNG state
        auto rng = std::make_unique<universal_rng_t>();
        
        constexpr size_t DEFAULT_BATCH_SIZE = 10000;
        int algo_type = (algorithm == RNG_ALGORITHM_XOROSHIRO) ? 0 : 1;
        
        OpenCLRNGState* opencl_state = create_opencl_rng(
            seed, 
            DEFAULT_BATCH_SIZE,  
            precision,
            algo_type
        );
        
        if (!opencl_state) {
            Logger::getInstance().log(Logger::Level::WARNING, 
                "OpenCL RNG initialization failed. Falling back to CPU implementation.");
            return nullptr;
        }
        
        // Populate universal RNG structure
        rng->state = opencl_state;
        rng->implementation_type = RNG_IMPL_OPENCL;
        rng->algorithm_type = algorithm;
        rng->precision_mode = precision;
        
        // Set function pointers
        rng->next_u64 = opencl_rng_next_u64;
        rng->next_double = opencl_rng_next_double;
        rng->free_func = opencl_rng_free;
        
        return rng;
    }

    // OpenCL-specific next_u64 implementation
    static uint64_t opencl_rng_next_u64(void* state) {
        OpenCLRNGState* opencl_state = static_cast<OpenCLRNGState*>(state);
        
        // Regenerate batch if needed
        if (opencl_state->current_batch_pos >= opencl_state->batch_size) {
            if (generate_random_batch(opencl_state) != 0) {
                Logger::getInstance().log(Logger::Level::ERROR, 
                    "Failed to generate OpenCL random batch");
                return 0;  // Fallback value
            }
            opencl_state->current_batch_pos = 0;
        }
        
        // Return next value from batch
        return static_cast<uint64_t*>(opencl_state->host_results)[opencl_state->current_batch_pos++];
    }

    // OpenCL-specific next_double implementation
    static double opencl_rng_next_double(void* state) {
        OpenCLRNGState* opencl_state = static_cast<OpenCLRNGState*>(state);
        
        // Ensure double precision
        if (opencl_state->precision_mode != RNG_PRECISION_DOUBLE) {
            Logger::getInstance().log(Logger::Level::WARNING, 
                "Requested double in non-double precision mode");
        }
        
        // Regenerate batch if needed
        if (opencl_state->current_batch_pos >= opencl_state->batch_size) {
            if (generate_random_batch(opencl_state) != 0) {
                Logger::getInstance().log(Logger::Level::ERROR, 
                    "Failed to generate OpenCL random batch");
                return 0.0;  // Fallback value
            }
            opencl_state->current_batch_pos = 0;
        }
        
        // Return next double from batch
        return static_cast<double*>(opencl_state->host_doubles)[opencl_state->current_batch_pos++];
    }

    // OpenCL-specific free function
    static void opencl_rng_free(void* state) {
        if (state) {
            free_opencl_rng(static_cast<OpenCLRNGState*>(state));
        }
    }

private:
    // Prevent instantiation
    OpenCLRNGIntegration() = delete;
    OpenCLRNGIntegration(const OpenCLRNGIntegration&) = delete;
    OpenCLRNGIntegration& operator=(const OpenCLRNGIntegration&) = delete;
};

// Convenience function for creating OpenCL Universal RNG
inline std::unique_ptr<universal_rng_t> create_opencl_universal_rng(
    uint64_t seed, 
    RNGAlgorithmType algorithm,
    RNGPrecisionMode precision
) {
    return OpenCLRNGIntegration::createUniversalRNG(seed, algorithm, precision);
}

#endif // OPENCL_RNG_INTEGRATION_H
