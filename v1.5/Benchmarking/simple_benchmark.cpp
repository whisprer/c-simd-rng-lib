#include "universal_rng.h"
#include "universal_rng_types.h"  // For enum definitions
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <string>
#include <algorithm>

// Simple benchmarking function
double benchmark_rng(universal_rng_t* rng, size_t iterations, bool use_batch = true) {
    auto start = std::chrono::high_resolution_clock::now();
    
    if (use_batch) {
        // Batch mode - generate numbers in batches for efficiency
        const size_t BATCH_SIZE = 10000;
        std::vector<uint64_t> results(BATCH_SIZE);
        
        for (size_t i = 0; i < iterations; i += BATCH_SIZE) {
            size_t count = std::min(BATCH_SIZE, iterations - i);
            universal_rng_generate_batch(rng, results.data(), count);
        }
    } else {
        // Single number generation mode
        uint64_t dummy = 0;
        for (size_t i = 0; i < iterations; i++) {
            dummy ^= universal_rng_next_u64(rng);
        }
        // Prevent compiler from optimizing away the loop
        if (dummy == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy << std::endl;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    return elapsed.count();
}

int main() {
    std::cout << "Universal RNG Benchmark" << std::endl;
    std::cout << "======================" << std::endl << std::endl;
    
    // Configuration
    const size_t NUM_ITERATIONS = 50000000;  // 50 million iterations
    const uint64_t SEED = 42;
    
    std::cout << "Running benchmarks with " << NUM_ITERATIONS << " iterations..." << std::endl << std::endl;
    
    // Benchmark Xoroshiro128++
    universal_rng_t* xoroshiro_rng = universal_rng_new(SEED, 0, 1); // 0 = Xoroshiro, 1 = Double precision
    if (!xoroshiro_rng) {
        std::cerr << "Failed to create Xoroshiro RNG!" << std::endl;
        return 1;
    }
    
    const char* xoroshiro_impl = universal_rng_get_implementation(xoroshiro_rng);
    std::cout << "Benchmarking " << xoroshiro_impl << std::endl;
    
    // Benchmark single generation
    double xoroshiro_single_time = benchmark_rng(xoroshiro_rng, NUM_ITERATIONS, false);
    double xoroshiro_single_rate = NUM_ITERATIONS / xoroshiro_single_time / 1'000'000;
    
    // Benchmark batch generation
    double xoroshiro_batch_time = benchmark_rng(xoroshiro_rng, NUM_ITERATIONS, true);
    double xoroshiro_batch_rate = NUM_ITERATIONS / xoroshiro_batch_time / 1'000'000;
    
    // Benchmark WyRand
    universal_rng_t* wyrand_rng = universal_rng_new(SEED, 1, 1); // 1 = WyRand, 1 = Double precision
    if (!wyrand_rng) {
        std::cerr << "Failed to create WyRand RNG!" << std::endl;
        universal_rng_free(xoroshiro_rng);
        universal_rng_free_string(xoroshiro_impl);
        return 1;
    }
    
    const char* wyrand_impl = universal_rng_get_implementation(wyrand_rng);
    std::cout << "Benchmarking " << wyrand_impl << std::endl;
    
    // Benchmark single generation
    double wyrand_single_time = benchmark_rng(wyrand_rng, NUM_ITERATIONS, false);
    double wyrand_single_rate = NUM_ITERATIONS / wyrand_single_time / 1'000'000;
    
    // Benchmark batch generation
    double wyrand_batch_time = benchmark_rng(wyrand_rng, NUM_ITERATIONS, true);
    double wyrand_batch_rate = NUM_ITERATIONS / wyrand_batch_time / 1'000'000;
    
    // Print results table
    std::cout << std::endl;
    std::cout << "Benchmark Results" << std::endl;
    std::cout << "================" << std::endl;
    std::cout << std::left << std::setw(25) << "Implementation" 
              << std::setw(15) << "Mode" 
              << std::setw(15) << "Time (sec)" 
              << std::setw(15) << "Speed (M/s)" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    std::cout << std::left << std::setw(25) << xoroshiro_impl
              << std::setw(15) << "Single"
              << std::fixed << std::setprecision(4) << std::setw(15) << xoroshiro_single_time
              << std::fixed << std::setprecision(2) << std::setw(15) << xoroshiro_single_rate << std::endl;
    
    std::cout << std::left << std::setw(25) << xoroshiro_impl
              << std::setw(15) << "Batch"
              << std::fixed << std::setprecision(4) << std::setw(15) << xoroshiro_batch_time
              << std::fixed << std::setprecision(2) << std::setw(15) << xoroshiro_batch_rate << std::endl;
    
    std::cout << std::left << std::setw(25) << wyrand_impl
              << std::setw(15) << "Single"
              << std::fixed << std::setprecision(4) << std::setw(15) << wyrand_single_time
              << std::fixed << std::setprecision(2) << std::setw(15) << wyrand_single_rate << std::endl;
    
    std::cout << std::left << std::setw(25) << wyrand_impl
              << std::setw(15) << "Batch"
              << std::fixed << std::setprecision(4) << std::setw(15) << wyrand_batch_time
              << std::fixed << std::setprecision(2) << std::setw(15) << wyrand_batch_rate << std::endl;
    
    // Speedup comparisons
    std::cout << std::endl << "Speedup Analysis" << std::endl;
    std::cout << "================" << std::endl;
    
    // Batch vs Single speedup
    double xoroshiro_batch_speedup = xoroshiro_single_time / xoroshiro_batch_time;
    double wyrand_batch_speedup = wyrand_single_time / wyrand_batch_time;
    
    std::cout << "Batch over Single generation speedup:" << std::endl;
    std::cout << "  " << xoroshiro_impl << ": " << std::fixed << std::setprecision(2) << xoroshiro_batch_speedup << "x" << std::endl;
    std::cout << "  " << wyrand_impl << ": " << std::fixed << std::setprecision(2) << wyrand_batch_speedup << "x" << std::endl;
    
    // Algorithm comparison
    double single_algo_speedup = wyrand_single_time / xoroshiro_single_time;
    double batch_algo_speedup = wyrand_batch_time / xoroshiro_batch_time;
    
    std::cout << std::endl << "Algorithm comparison (Xoroshiro vs WyRand):" << std::endl;
    std::cout << "  Single mode: " << (single_algo_speedup > 1.0 ? "Xoroshiro faster by " : "WyRand faster by ")
              << std::fixed << std::setprecision(2) << std::abs(single_algo_speedup - 1.0) * 100 << "%" << std::endl;
    std::cout << "  Batch mode: " << (batch_algo_speedup > 1.0 ? "Xoroshiro faster by " : "WyRand faster by ")
              << std::fixed << std::setprecision(2) << std::abs(batch_algo_speedup - 1.0) * 100 << "%" << std::endl;
    
    // Cleanup
    universal_rng_free(xoroshiro_rng);
    universal_rng_free(wyrand_rng);
    universal_rng_free_string(xoroshiro_impl);
    universal_rng_free_string(wyrand_impl);
    
    std::cout << std::endl << "Benchmark completed successfully!" << std::endl;
    return 0;
}