#include "universal_rng.h"
#include "universal_rng_types.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <string>
#include <algorithm>
#include <random>  // For std::mt19937_64

// Basic Xoroshiro128+ implementation (similar to what might be in C++ std lib)
class Xoroshiro128Plus {
private:
    uint64_t s[2];

    static inline uint64_t rotl(const uint64_t x, int k) {
        return (x << k) | (x >> (64 - k));
    }

public:
    Xoroshiro128Plus(uint64_t seed) {
        // Initialize with SplitMix64
        s[0] = seed;
        s[1] = seed + 0x9e3779b97f4a7c15ULL;
        
        for (int i = 0; i < 4; i++) {
            s[0] = 0x9e3779b97f4a7c15ULL + s[0];
            s[0] = (s[0] ^ (s[0] >> 30)) * 0xbf58476d1ce4e5b9ULL;
            s[0] = (s[0] ^ (s[0] >> 27)) * 0x94d049bb133111ebULL;
            s[0] = s[0] ^ (s[0] >> 31);
            
            s[1] = 0x9e3779b97f4a7c15ULL + s[1];
            s[1] = (s[1] ^ (s[1] >> 30)) * 0xbf58476d1ce4e5b9ULL;
            s[1] = (s[1] ^ (s[1] >> 27)) * 0x94d049bb133111ebULL;
            s[1] = s[1] ^ (s[1] >> 31);
        }
    }

    uint64_t next() {
        const uint64_t s0 = s[0];
        uint64_t s1 = s[1];
        const uint64_t result = s0 + s1;

        s1 ^= s0;
        s[0] = rotl(s0, 24) ^ s1 ^ (s1 << 16); // a, b
        s[1] = rotl(s1, 37); // c

        return result;
    }

    double next_double() {
        return (next() >> 11) * (1.0 / (1ULL << 53));
    }
};

// Simple benchmarking function for Universal RNG
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

// Benchmarking for std::mt19937_64
double benchmark_std_mt(std::mt19937_64& rng, size_t iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    
    uint64_t dummy = 0;
    for (size_t i = 0; i < iterations; i++) {
        dummy ^= rng();
    }
    
    // Prevent compiler from optimizing away the loop
    if (dummy == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy << std::endl;
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    return elapsed.count();
}

// Benchmarking for Xoroshiro128Plus
double benchmark_xoroshiro128plus(Xoroshiro128Plus& rng, size_t iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    
    uint64_t dummy = 0;
    for (size_t i = 0; i < iterations; i++) {
        dummy ^= rng.next();
    }
    
    // Prevent compiler from optimizing away the loop
    if (dummy == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy << std::endl;
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    return elapsed.count();
}

// Struct to hold benchmark results
struct BenchResult {
    std::string name;
    std::string mode;
    double time;
    double rate;
};

int main() {
    std::cout << "Universal RNG Benchmark" << std::endl;
    std::cout << "======================" << std::endl << std::endl;
    
    // Configuration
    const size_t NUM_ITERATIONS = 50000000;  // 50 million iterations
    const uint64_t SEED = 42;
    std::vector<BenchResult> results;
    
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
    results.push_back({xoroshiro_impl, "Single", xoroshiro_single_time, xoroshiro_single_rate});
    
    // Benchmark batch generation
    double xoroshiro_batch_time = benchmark_rng(xoroshiro_rng, NUM_ITERATIONS, true);
    double xoroshiro_batch_rate = NUM_ITERATIONS / xoroshiro_batch_time / 1'000'000;
    results.push_back({xoroshiro_impl, "Batch", xoroshiro_batch_time, xoroshiro_batch_rate});
    
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
    results.push_back({wyrand_impl, "Single", wyrand_single_time, wyrand_single_rate});
    
    // Benchmark batch generation
    double wyrand_batch_time = benchmark_rng(wyrand_rng, NUM_ITERATIONS, true);
    double wyrand_batch_rate = NUM_ITERATIONS / wyrand_batch_time / 1'000'000;
    results.push_back({wyrand_impl, "Batch", wyrand_batch_time, wyrand_batch_rate});
    
    // Benchmark std::mt19937_64 (Mersenne Twister)
    std::mt19937_64 mt_rng(SEED);
    std::cout << "Benchmarking std::mt19937_64" << std::endl;
    double mt_time = benchmark_std_mt(mt_rng, NUM_ITERATIONS);
    double mt_rate = NUM_ITERATIONS / mt_time / 1'000'000;
    results.push_back({"std::mt19937_64", "Single", mt_time, mt_rate});
    
    // Benchmark basic Xoroshiro128+
    Xoroshiro128Plus xoro_plus(SEED);
    std::cout << "Benchmarking Xoroshiro128+" << std::endl;
    double xoro_plus_time = benchmark_xoroshiro128plus(xoro_plus, NUM_ITERATIONS);
    double xoro_plus_rate = NUM_ITERATIONS / xoro_plus_time / 1'000'000;
    results.push_back({"Xoroshiro128+", "Single", xoro_plus_time, xoro_plus_rate});
    
    // Print results table
    std::cout << std::endl;
    std::cout << "Benchmark Results" << std::endl;
    std::cout << "================" << std::endl;
    std::cout << std::left << std::setw(25) << "Implementation" 
              << std::setw(15) << "Mode" 
              << std::setw(15) << "Time (sec)" 
              << std::setw(15) << "Speed (M/s)" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    for (const auto& result : results) {
        std::cout << std::left << std::setw(25) << result.name
                  << std::setw(15) << result.mode
                  << std::fixed << std::setprecision(4) << std::setw(15) << result.time
                  << std::fixed << std::setprecision(2) << std::setw(15) << result.rate << std::endl;
    }
    
    // Speedup comparisons
    std::cout << std::endl << "Speedup Analysis" << std::endl;
    std::cout << "================" << std::endl;
    
    // Batch vs Single speedup
    double xoroshiro_batch_speedup = xoroshiro_single_time / xoroshiro_batch_time;
    double wyrand_batch_speedup = wyrand_single_time / wyrand_batch_time;
    
    std::cout << "Batch over Single generation speedup:" << std::endl;
    std::cout << "  " << xoroshiro_impl << ": " << std::fixed << std::setprecision(2) << xoroshiro_batch_speedup << "x" << std::endl;
    std::cout << "  " << wyrand_impl << ": " << std::fixed << std::setprecision(2) << wyrand_batch_speedup << "x" << std::endl;
    
    // Compare with std::mt19937_64
    double mt_vs_xoroshiro = mt_time / xoroshiro_single_time;
    double mt_vs_wyrand = mt_time / wyrand_single_time;
    double mt_vs_batch_xoroshiro = mt_time / xoroshiro_batch_time;
    double mt_vs_batch_wyrand = mt_time / wyrand_batch_time;
    
    std::cout << std::endl << "Compared to std::mt19937_64:" << std::endl;
    std::cout << "  " << xoroshiro_impl << " (Single): " << std::fixed << std::setprecision(2) << mt_vs_xoroshiro << "x faster" << std::endl;
    std::cout << "  " << wyrand_impl << " (Single): " << std::fixed << std::setprecision(2) << mt_vs_wyrand << "x faster" << std::endl;
    std::cout << "  " << xoroshiro_impl << " (Batch): " << std::fixed << std::setprecision(2) << mt_vs_batch_xoroshiro << "x faster" << std::endl;
    std::cout << "  " << wyrand_impl << " (Batch): " << std::fixed << std::setprecision(2) << mt_vs_batch_wyrand << "x faster" << std::endl;
    
    // Compare with Xoroshiro128+
    double xoro_plus_vs_xoroshiro = xoro_plus_time / xoroshiro_single_time;
    double xoro_plus_vs_wyrand = xoro_plus_time / wyrand_single_time;
    double xoro_plus_vs_batch_xoroshiro = xoro_plus_time / xoroshiro_batch_time;
    double xoro_plus_vs_batch_wyrand = xoro_plus_time / wyrand_batch_time;
    
    std::cout << std::endl << "Compared to Xoroshiro128+:" << std::endl;
    std::cout << "  " << xoroshiro_impl << " (Single): " << std::fixed << std::setprecision(2) << xoro_plus_vs_xoroshiro << "x faster" << std::endl;
    std::cout << "  " << wyrand_impl << " (Single): " << std::fixed << std::setprecision(2) << xoro_plus_vs_wyrand << "x faster" << std::endl;
    std::cout << "  " << xoroshiro_impl << " (Batch): " << std::fixed << std::setprecision(2) << xoro_plus_vs_batch_xoroshiro << "x faster" << std::endl;
    std::cout << "  " << wyrand_impl << " (Batch): " << std::fixed << std::setprecision(2) << xoro_plus_vs_batch_wyrand << "x faster" << std::endl;
    
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