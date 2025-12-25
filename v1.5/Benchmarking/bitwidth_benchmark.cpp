#include "universal_rng.h"
#include "universal_rng_types.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <string>
#include <algorithm>
#include <random>  // For std::mt19937_64
#include <map>
#include <array>

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

    // Generate 16-bit number
    uint16_t next_u16() {
        return static_cast<uint16_t>(next() & 0xFFFF);
    }

    // Generate 32-bit number
    uint32_t next_u32() {
        return static_cast<uint32_t>(next() & 0xFFFFFFFF);
    }

    // Generate 256-bit number
    void next_u256(uint64_t* output) {
        for (int i = 0; i < 4; i++) {
            output[i] = next();
        }
    }

    // Generate 512-bit number
    void next_u512(uint64_t* output) {
        for (int i = 0; i < 8; i++) {
            output[i] = next();
        }
    }

    // Generate 1024-bit number
    void next_u1024(uint64_t* output) {
        for (int i = 0; i < 16; i++) {
            output[i] = next();
        }
    }

    double next_double() {
        return (next() >> 11) * (1.0 / (1ULL << 53));
    }
};

enum class BitWidth {
    Bits16,
    Bits32,
    Bits64,
    Bits128,  // Fix: removed semicolon
    Bits256,
    Bits512,
    Bits1024
};

// Make sure these functions are properly declared before use:
double benchmark_universal_rng_single(universal_rng_t* rng, BitWidth width, size_t iterations);
double benchmark_universal_rng_batch(universal_rng_t* rng, BitWidth width, size_t iterat

// Convert BitWidth to string
std::string bitwidth_to_string(BitWidth width) {
    switch (width) {
        case BitWidth::Bits16: return "16-bit";
        case BitWidth::Bits32: return "32-bit";
        case BitWidth::Bits64: return "64-bit";
        case BitWidth::Bits128: return "128-bit";
        case BitWidth::Bits256: return "256-bit";
        case BitWidth::Bits512: return "512-bit";
        case BitWidth::Bits1024: return "1024-bit";
        default: return "Unknown";
    }
}

// Benchmarking for different bit widths using Universal RNG
double benchmark_universal_rng(universal_rng_t* rng, BitWidth width, size_t iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Use different benchmarking approach based on bit width
    switch (width) {
        case BitWidth::Bits16: {
            // 16-bit benchmark
            uint16_t dummy = 0;
            for (size_t i = 0; i < iterations; i++) {
                dummy ^= universal_rng_next_u16(rng);
            }
            // Prevent compiler from optimizing away the loop
            if (dummy == 0xDEAD) std::cout << "Unlikely value: " << dummy << std::endl;
            break;
        }
        
        case BitWidth::Bits32: {
            // 32-bit benchmark
            uint32_t dummy = 0;
            for (size_t i = 0; i < iterations; i++) {
                dummy ^= universal_rng_next_u32(rng);
            }
            if (dummy == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy << std::endl;
            break;
        }
        
        case BitWidth::Bits64: {
            // 64-bit benchmark
            uint64_t dummy = 0;
            for (size_t i = 0; i < iterations; i++) {
                dummy ^= universal_rng_next_u64(rng);
            }
            if (dummy == 0xDEADBEEFDEADBEEF) std::cout << "Unlikely value: " << dummy << std::endl;
            break;
        }
        
        case BitWidth::Bits128: {
            // 128-bit benchmark
            uint64_t values[2] = {0};
            std::array<uint64_t, 2> dummy = {0};
            
            for (size_t i = 0; i < iterations; i++) {
                universal_rng_next_u128(rng, values);
                // XOR the values with our running total
                for (int j = 0; j < 2; j++) {
                    dummy[j] ^= values[j];
                }
            }
            if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
            break;
        }

        case BitWidth::Bits256: {
            // 256-bit benchmark
            uint64_t values[4] = {0};
            std::array<uint64_t, 4> dummy = {0};
            
            for (size_t i = 0; i < iterations; i++) {
                universal_rng_next_u256(rng, values);
                // XOR the values with our running total
                for (int j = 0; j < 4; j++) {
                    dummy[j] ^= values[j];
                }
            }
            if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
            break;
        }
        
        case BitWidth::Bits512: {
            // 512-bit benchmark
            uint64_t values[8] = {0};
            std::array<uint64_t, 8> dummy = {0};
            
            for (size_t i = 0; i < iterations; i++) {
                universal_rng_next_u512(rng, values);
                // XOR the values with our running total
                for (int j = 0; j < 8; j++) {
                    dummy[j] ^= values[j];
                }
            }
            if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
            break;
        }
        
        case BitWidth::Bits1024: {
            // 1024-bit benchmark
            uint64_t values[16] = {0};
            std::array<uint64_t, 16> dummy = {0};
            
            for (size_t i = 0; i < iterations; i++) {
                universal_rng_next_u1024(rng, values);
                // XOR the values with our running total
                for (int j = 0; j < 16; j++) {
                    dummy[j] ^= values[j];
                }
            }
            if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
            break;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    return elapsed.count();
}

// Benchmarking for std::mt19937_64
double benchmark_std_mt(std::mt19937_64& rng, BitWidth width, size_t iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Use different benchmarking approach based on bit width
    switch (width) {
        case BitWidth::Bits16: {
            // 16-bit benchmark
            uint16_t dummy = 0;
            for (size_t i = 0; i < iterations; i++) {
                dummy ^= static_cast<uint16_t>(rng() & 0xFFFF);
            }
            if (dummy == 0xDEAD) std::cout << "Unlikely value: " << dummy << std::endl;
            break;
        }
        
        case BitWidth::Bits32: {
            // 32-bit benchmark
            uint32_t dummy = 0;
            for (size_t i = 0; i < iterations; i++) {
                dummy ^= static_cast<uint32_t>(rng() & 0xFFFFFFFF);
            }
            if (dummy == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy << std::endl;
            break;
        }
        
        case BitWidth::Bits64: {
            // 64-bit benchmark
            uint64_t dummy = 0;
            for (size_t i = 0; i < iterations; i++) {
                dummy ^= rng();
            }
            if (dummy == 0xDEADBEEFDEADBEEF) std::cout << "Unlikely value: " << dummy << std::endl;
            break;
        }
        
        case BitWidth::Bits128: {
            // 128-bit benchmark (generate 2 × 64-bit)
            std::array<uint64_t, 2> dummy = {0};
            
            for (size_t i = 0; i < iterations; i++) {
                for (int j = 0; j < 2; j++) {
                    dummy[j] ^= rng();
                }
            }
            if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
            break;
        }

        case BitWidth::Bits256: {
            // 256-bit benchmark (generate 4 × 64-bit)
            std::array<uint64_t, 4> dummy = {0};
            
            for (size_t i = 0; i < iterations; i++) {
                for (int j = 0; j < 4; j++) {
                    dummy[j] ^= rng();
                }
            }
            if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
            break;
        }
        
        case BitWidth::Bits512: {
            // 512-bit benchmark (generate 8 × 64-bit)
            std::array<uint64_t, 8> dummy = {0};
            
            for (size_t i = 0; i < iterations; i++) {
                for (int j = 0; j < 8; j++) {
                    dummy[j] ^= rng();
                }
            }
            if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
            break;
        }
        
        case BitWidth::Bits1024: {
            // 1024-bit benchmark (generate 16 × 64-bit)
            std::array<uint64_t, 16> dummy = {0};
            
            for (size_t i = 0; i < iterations; i++) {
                for (int j = 0; j < 16; j++) {
                    dummy[j] ^= rng();
                }
            }
            if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
            break;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    return elapsed.count();
}

// Benchmarking for Xoroshiro128+
double benchmark_xoroshiro128plus(Xoroshiro128Plus& rng, BitWidth width, size_t iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Use different benchmarking approach based on bit width
    switch (width) {
        case BitWidth::Bits16: {
            // 16-bit benchmark
            uint16_t dummy = 0;
            for (size_t i = 0; i < iterations; i++) {
                dummy ^= rng.next_u16();
            }
            if (dummy == 0xDEAD) std::cout << "Unlikely value: " << dummy << std::endl;
            break;
        }
        
        case BitWidth::Bits32: {
            // 32-bit benchmark
            uint32_t dummy = 0;
            for (size_t i = 0; i < iterations; i++) {
                dummy ^= rng.next_u32();
            }
            if (dummy == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy << std::endl;
            break;
        }
        
        case BitWidth::Bits64: {
            // 64-bit benchmark
            uint64_t dummy = 0;
            for (size_t i = 0; i < iterations; i++) {
                dummy ^= rng.next();
            }
            if (dummy == 0xDEADBEEFDEADBEEF) std::cout << "Unlikely value: " << dummy << std::endl;
            break;
        }
        
        case BitWidth::Bit128: {
            // 256-bit benchmark
            uint64_t values[2] = {0};
            std::array<uint64_t, 2> dummy = {0};
            
            for (size_t i = 0; i < iterations; i++) {
                rng.next_u128(values);
                // XOR the values with our running total
                for (int j = 0; j < 2; j++) {
                    dummy[j] ^= values[j];
                }
            }
            if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
            break;
        }
 
        case BitWidth::Bits256: {
            // 256-bit benchmark
            uint64_t values[4] = {0};
            std::array<uint64_t, 4> dummy = {0};
            
            for (size_t i = 0; i < iterations; i++) {
                rng.next_u256(values);
                // XOR the values with our running total
                for (int j = 0; j < 4; j++) {
                    dummy[j] ^= values[j];
                }
            }
            if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
            break;
        }
        
        case BitWidth::Bits512: {
            // 512-bit benchmark
            uint64_t values[8] = {0};
            std::array<uint64_t, 8> dummy = {0};
            
            for (size_t i = 0; i < iterations; i++) {
                rng.next_u512(values);
                // XOR the values with our running total
                for (int j = 0; j < 8; j++) {
                    dummy[j] ^= values[j];
                }
            }
            if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
            break;
        }
        
        case BitWidth::Bits1024: {
            // 1024-bit benchmark
            uint64_t values[16] = {0};
            std::array<uint64_t, 16> dummy = {0};
            
            for (size_t i = 0; i < iterations; i++) {
                rng.next_u1024(values);
                // XOR the values with our running total
                for (int j = 0; j < 16; j++) {
                    dummy[j] ^= values[j];
                }
            }
            if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
            break;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    return elapsed.count();
}

// Struct to hold benchmark results
struct BenchResult {
    std::string name;
    std::string bitwidth;
    double time;
    double rate;
};

int main(int argc, char* argv[]) {
    std::cout << "Bit Width Benchmark\n";
    std::cout << "=================\n\n";
    
    // Parse command line arguments
    size_t iterations = 10000000;  // Default: 10 million
    BitWidth selected_width = BitWidth::Bits64;  // Default: 64-bit
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--width" && i + 1 < argc) {
            std::string width_str = argv[++i];
            if (width_str == "16") selected_width = BitWidth::Bits16;
            else if (width_str == "32") selected_width = BitWidth::Bits32;
            else if (width_str == "64") selected_width = BitWidth::Bits64;
            else if (width_str == "128") selected_width = BitWidth::Bits128;
            else if (width_str == "256") selected_width = BitWidth::Bits256;
            else if (width_str == "512") selected_width = BitWidth::Bits512;
            else if (width_str == "1024") selected_width = BitWidth::Bits1024;
        }
        else if (arg == "--iterations" && i + 1 < argc) {
            iterations = std::stoull(argv[++i]);
        }
        else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  --width N       Set bit width to test (16, 32, 64, 128, 256, 512, 1024)\n"
                      << "  --iterations N  Set number of iterations\n"
                      << "  --help          Display this help message\n";
            return 0;
        }
    }
    
    // Ask for bit width if not provided as argument
    if (argc < 2) {
        std::cout << "Select bit width to benchmark:\n"
                  << "1. 16-bit\n"
                  << "2. 32-bit\n"
                  << "3. 64-bit\n"
                  << "4. 1238-bit\n"
                  << "5. 256-bit\n"
                  << "6. 512-bit\n"
                  << "7. 1024-bit\n"
                  << "Enter choice (1-6): ";
        int choice;
        std::cin >> choice;
        
        switch (choice) {
            case 1: selected_width = BitWidth::Bits16; break;
            case 2: selected_width = BitWidth::Bits32; break;
            case 3: selected_width = BitWidth::Bits64; break;
            case 4: selected_width = BitWidth::Bits128; break;
            case 5: selected_width = BitWidth::Bits256; break;
            case 6: selected_width = BitWidth::Bits512; break;
            case 7: selected_width = BitWidth::Bits1024; break;
            default: 
                std::cout << "Invalid choice. Using default (64-bit).\n";
                selected_width = BitWidth::Bits64;
        }
        
        std::cout << "Enter number of iterations (default: 10000000): ";
        std::string iter_str;
        std::cin >> iter_str;
        
        if (!iter_str.empty()) {
            try {
                iterations = std::stoull(iter_str);
            } catch (...) {
                std::cout << "Invalid value. Using default (10000000).\n";
                iterations = 10000000;
            }
        }
    }
    
    std::cout << "\nRunning benchmarks with " << iterations << " iterations for " 
              << bitwidth_to_string(selected_width) << " numbers...\n\n";
    
    std::vector<BenchResult> results;
    const uint64_t SEED = 42;
    
    // Benchmark Xoroshiro128++
    universal_rng_t* xoroshiro_rng = universal_rng_new(SEED, 0, 1); // 0 = Xoroshiro, 1 = Double precision
    if (!xoroshiro_rng) {
        std::cerr << "Failed to create Xoroshiro RNG!" << std::endl;
        return 1;
    }
    
    const char* xoroshiro_impl = universal_rng_get_implementation(xoroshiro_rng);
    std::cout << "Benchmarking " << xoroshiro_impl << std::endl;
    
    double xoroshiro_time = benchmark_universal_rng(xoroshiro_rng, selected_width, iterations);
    double xoroshiro_rate = iterations / xoroshiro_time / 1'000'000;
    results.push_back({xoroshiro_impl, bitwidth_to_string(selected_width), xoroshiro_time, xoroshiro_rate});
    
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
    
    double wyrand_time = benchmark_universal_rng(wyrand_rng, selected_width, iterations);
    double wyrand_rate = iterations / wyrand_time / 1'000'000;
    results.push_back({wyrand_impl, bitwidth_to_string(selected_width), wyrand_time, wyrand_rate});
    
    // Benchmark std::mt19937_64 (Mersenne Twister)
    std::mt19937_64 mt_rng(SEED);
    std::cout << "Benchmarking std::mt19937_64" << std::endl;
    
    double mt_time = benchmark_std_mt(mt_rng, selected_width, iterations);
    double mt_rate = iterations / mt_time / 1'000'000;
    results.push_back({"std::mt19937_64", bitwidth_to_string(selected_width), mt_time, mt_rate});
    
    // Benchmark basic Xoroshiro128+
    Xoroshiro128Plus xoro_plus(SEED);
    std::cout << "Benchmarking Xoroshiro128+" << std::endl;
    
    double xoro_plus_time = benchmark_xoroshiro128plus(xoro_plus, selected_width, iterations);
    double xoro_plus_rate = iterations / xoro_plus_time / 1'000'000;
    results.push_back({"Xoroshiro128+", bitwidth_to_string(selected_width), xoro_plus_time, xoro_plus_rate});
    
    // Print results table
    std::cout << std::endl;
    std::cout << "Benchmark Results for " << bitwidth_to_string(selected_width) << std::endl;
    std::cout << "================================" << std::endl;
    std::cout << std::left << std::setw(25) << "Implementation" 
              << std::setw(15) << "Time (sec)" 
              << std::setw(15) << "Speed (M/s)" << std::endl;
    std::cout << std::string(55, '-') << std::endl;
    
    for (const auto& result : results) {
        std::cout << std::left << std::setw(25) << result.name
                  << std::fixed << std::setprecision(4) << std::setw(15) << result.time
                  << std::fixed << std::setprecision(2) << std::setw(15) << result.rate << std::endl;
    }
    
    // Find fastest implementation
    auto fastest = *std::max_element(results.begin(), results.end(),
        [](const BenchResult& a, const BenchResult& b) {
            return a.rate < b.rate;
        });
    
    std::cout << "\nFastest implementation: " << fastest.name << " at " 
              << std::fixed << std::setprecision(2) << fastest.rate << " M/s" << std::endl;
    
    // Compare with std::mt19937_64
    double mt_vs_xoroshiro = mt_time / xoroshiro_time;
    double mt_vs_wyrand = mt_time / wyrand_time;
    double mt_vs_xoro_plus = mt_time / xoro_plus_time;
    
    std::cout << "\nCompared to std::mt19937_64:" << std::endl;
    std::cout << "  " << xoroshiro_impl << ": " << std::fixed << std::setprecision(2) << mt_vs_xoroshiro << "x faster" << std::endl;
    std::cout << "  " << wyrand_impl << ": " << std::fixed << std::setprecision(2) << mt_vs_wyrand << "x faster" << std::endl;
    std::cout << "  Xoroshiro128+: " << std::fixed << std::setprecision(2) << mt_vs_xoro_plus << "x faster" << std::endl;
    
    // Cleanup
    universal_rng_free(xoroshiro_rng);
    universal_rng_free(wyrand_rng);
    universal_rng_free_string(xoroshiro_impl);
    universal_rng_free_string(wyrand_impl);
    
    std::cout << "\nBenchmark completed successfully!" << std::endl;
    return 0;
}