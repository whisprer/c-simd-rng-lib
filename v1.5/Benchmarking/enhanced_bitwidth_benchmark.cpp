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

// Enum for benchmark types
enum class BitWidth {
    Bits16,
    Bits32,
    Bits64,
    Bits128,
    Bits256,
    Bits512,
    Bits1024
};

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

// Define batch sizes for different bit widths
size_t get_batch_size(BitWidth width) {
    switch (width) {
        case BitWidth::Bits16: return 10000;
        case BitWidth::Bits32: return 10000;
        case BitWidth::Bits64: return 10000;
        case BitWidth::Bits128: return 1000;
        case BitWidth::Bits256: return 1000;
        case BitWidth::Bits512: return 1000;
        case BitWidth::Bits1024: return 500;
        default: return 1000;
    }
}

// Basic Xoroshiro128+ implementation (similar to what might be in C++ std lib)
class Xoroshiro128Plus {
public:
    uint64_t s[2];

    static inline uint64_t rotl(const uint64_t x, int k) {
        return (x << k) | (x >> (64 - k));
    }

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

    // Generate 128-bit number
    void next_u128(uint64_t* output) {
        for (int i = 0; i < 2; i++) {
            output[i] = next();
        }
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

    // Batch generation for Xoroshiro128+
    void generate_batch_u16(uint16_t* results, size_t count) {
        for (size_t i = 0; i < count; i++) {
            results[i] = next_u16();
        }
    }

    void generate_batch_u32(uint32_t* results, size_t count) {
        for (size_t i = 0; i < count; i++) {
            results[i] = next_u32();
        }
    }

    void generate_batch_u64(uint64_t* results, size_t count) {
        for (size_t i = 0; i < count; i++) {
            results[i] = next();
        }
    }

    void generate_batch_u128(uint64_t* results, size_t count) {
        for (size_t i = 0; i < count * 2; i += 2) {
            next_u128(&results[i]);
        }
    }

    void generate_batch_u256(uint64_t* results, size_t count) {
        for (size_t i = 0; i < count * 4; i += 4) {
            next_u256(&results[i]);
        }
    }

    void generate_batch_u512(uint64_t* results, size_t count) {
        for (size_t i = 0; i < count * 8; i += 8) {
            next_u512(&results[i]);
        }
    }

    void generate_batch_u1024(uint64_t* results, size_t count) {
        for (size_t i = 0; i < count * 16; i += 16) {
            next_u1024(&results[i]);
        }
    }

    double next_double() {
        return (next() >> 11) * (1.0 / (1ULL << 53));
    }
};

// Struct to hold benchmark results
struct BenchResult {
    std::string name;
    std::string mode;
    std::string bitwidth;
    double time;
    double rate;
};

// Function declarations
double benchmark_universal_rng_single(universal_rng_t* rng, BitWidth width, size_t iterations);
double benchmark_universal_rng_batch(universal_rng_t* rng, BitWidth width, size_t iterations);
double benchmark_std_mt_single(std::mt19937_64& rng, BitWidth width, size_t iterations);
double benchmark_std_mt_batch(std::mt19937_64& rng, BitWidth width, size_t iterations);
double benchmark_xoroshiro128plus_single(Xoroshiro128Plus& rng, BitWidth width, size_t iterations);
double benchmark_xoroshiro128plus_batch(Xoroshiro128Plus& rng, BitWidth width, size_t iterations);

// Benchmarking for different bit widths using Universal RNG (Single mode)
double benchmark_universal_rng_single(universal_rng_t* rng, BitWidth width, size_t iterations) {
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

// Benchmarking for different bit widths using Universal RNG (Batch mode)
double benchmark_universal_rng_batch(universal_rng_t* rng, BitWidth width, size_t iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    size_t batch_size = get_batch_size(width);
    size_t batch_count = (iterations + batch_size - 1) / batch_size;
    
    // Use different benchmarking approach based on bit width
    switch (width) {
        case BitWidth::Bits16: {
            // 16-bit batch benchmark
            std::vector<uint16_t> batch(batch_size);
            uint16_t dummy = 0;
            
            for (size_t b = 0; b < batch_count; b++) {
                size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ? 
                                          iterations % batch_size : batch_size;
                
                universal_rng_generate_batch_u16(rng, batch.data(), this_batch_size);
                
                // XOR with dummy for side effect to prevent optimization
                for (size_t i = 0; i < this_batch_size; i++) {
                    dummy ^= batch[i];
                }
            }
            if (dummy == 0xDEAD) std::cout << "Unlikely value: " << dummy << std::endl;
            break;
        }
        
        case BitWidth::Bits32: {
            // 32-bit batch benchmark
            std::vector<uint32_t> batch(batch_size);
            uint32_t dummy = 0;
            
            for (size_t b = 0; b < batch_count; b++) {
                size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ? 
                                          iterations % batch_size : batch_size;
                
                universal_rng_generate_batch_u32(rng, batch.data(), this_batch_size);
                
                // XOR with dummy for side effect to prevent optimization
                for (size_t i = 0; i < this_batch_size; i++) {
                    dummy ^= batch[i];
                }
            }
            if (dummy == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy << std::endl;
            break;
        }
        
        case BitWidth::Bits64: {
            // 64-bit batch benchmark
            std::vector<uint64_t> batch(batch_size);
            uint64_t dummy = 0;
            
            for (size_t b = 0; b < batch_count; b++) {
                size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ? 
                                          iterations % batch_size : batch_size;
                
                universal_rng_generate_batch(rng, batch.data(), this_batch_size);
                
                // XOR with dummy for side effect to prevent optimization
                for (size_t i = 0; i < this_batch_size; i++) {
                    dummy ^= batch[i];
                }
            }
            if (dummy == 0xDEADBEEFDEADBEEF) std::cout << "Unlikely value: " << dummy << std::endl;
            break;
        }

        case BitWidth::Bits128: {
            // 128-bit batch benchmark
            size_t u64_count = batch_size * 2; // each 128-bit number is 2 uint64_t
            std::vector<uint64_t> batch(u64_count);
            std::array<uint64_t, 2> dummy = {0};

            for (size_t b = 0; b < batch_count; b++) {
                size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ?
                                        iterations % batch_size : batch_size;

                universal_rng_generate_batch_u128(rng, batch.data(), this_batch_size);

                // XOR with dummy for side effect to prevent optimization
                for (size_t i = 0; i < this_batch_size; i++) {
                    for (int j = 0; j < 2; j++) {
                        dummy[j] ^= batch[i*2 + j];
                    }
                }    
            }
            if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
            break;
        }

        case BitWidth::Bits256: {
            // 256-bit batch benchmark
            size_t u64_count = batch_size * 4; // Each 256-bit number is 4 uint64_t
            std::vector<uint64_t> batch(u64_count);
            std::array<uint64_t, 4> dummy = {0};
            
            for (size_t b = 0; b < batch_count; b++) {
                size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ? 
                                          iterations % batch_size : batch_size;
                
                universal_rng_generate_batch_u256(rng, batch.data(), this_batch_size);
                
                // XOR with dummy for side effect to prevent optimization
                for (size_t i = 0; i < this_batch_size; i++) {
                    for (int j = 0; j < 4; j++) {
                        dummy[j] ^= batch[i*4 + j];
                    }
                }
            }
            if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
            break;
        }
        
        case BitWidth::Bits512: {
            // 512-bit batch benchmark
            size_t u64_count = batch_size * 8; // Each 512-bit number is 8 uint64_t
            std::vector<uint64_t> batch(u64_count);
            std::array<uint64_t, 8> dummy = {0};
            
            for (size_t b = 0; b < batch_count; b++) {
                size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ? 
                                          iterations % batch_size : batch_size;
                
                universal_rng_generate_batch_u512(rng, batch.data(), this_batch_size);
                
                // XOR with dummy for side effect to prevent optimization
                for (size_t i = 0; i < this_batch_size; i++) {
                    for (int j = 0; j < 8; j++) {
                        dummy[j] ^= batch[i*8 + j];
                    }
                }
            }
            if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
            break;
        }
        
        case BitWidth::Bits1024: {
            // 1024-bit batch benchmark
            size_t u64_count = batch_size * 16; // Each 1024-bit number is 16 uint64_t
            std::vector<uint64_t> batch(u64_count);
            std::array<uint64_t, 16> dummy = {0};
            
            for (size_t b = 0; b < batch_count; b++) {
                size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ? 
                                          iterations % batch_size : batch_size;
                
                universal_rng_generate_batch_u1024(rng, batch.data(), this_batch_size);
                
                // XOR with dummy for side effect to prevent optimization
                for (size_t i = 0; i < this_batch_size; i++) {
                    for (int j = 0; j < 16; j++) {
                        dummy[j] ^= batch[i*16 + j];
                    }
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

// Benchmarking for std::mt19937_64 (single mode)
double benchmark_std_mt_single(std::mt19937_64& rng, BitWidth width, size_t iterations) {
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
            // 128-bit benchmark (generate 2 x 64-bit)
            std::array<uint64_t, 2> dummy = {0};
            
            for (size_t i = 0; i < iterations; i++) {
                for (int j = 0; j < 2; j++) {
                    dummy[j] ^= rng();
                }
            }
            if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely values: " << dummy[0] << std::endl;
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

// Benchmarking for std::mt19937_64 (batch mode)
double benchmark_std_mt_batch(std::mt19937_64& rng, BitWidth width, size_t iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    size_t batch_size = get_batch_size(width);
    size_t batch_count = (iterations + batch_size - 1) / batch_size;
    
    // Use different benchmarking approach based on bit width
    switch (width) {
        case BitWidth::Bits16: {
            // 16-bit batch benchmark
            std::vector<uint16_t> batch(batch_size);
            uint16_t dummy = 0;
            
            for (size_t b = 0; b < batch_count; b++) {
                size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ? 
                                          iterations % batch_size : batch_size;
                
                // Generate batch
                for (size_t i = 0; i < this_batch_size; i++) {
                    batch[i] = static_cast<uint16_t>(rng() & 0xFFFF);
                }
                
                // XOR with dummy for side effect to prevent optimization
                for (size_t i = 0; i < this_batch_size; i++) {
                    dummy ^= batch[i];
                }
            }
            if (dummy == 0xDEAD) std::cout << "Unlikely value: " << dummy << std::endl;
            break;
        }
        
        case BitWidth::Bits32: {
            // 32-bit batch benchmark
            std::vector<uint32_t> batch(batch_size);
            uint32_t dummy = 0;
            
            for (size_t b = 0; b < batch_count; b++) {
                size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ? 
                                          iterations % batch_size : batch_size;
                
                // Generate batch
                for (size_t i = 0; i < this_batch_size; i++) {
                    batch[i] = static_cast<uint32_t>(rng() & 0xFFFFFFFF);
                }
                
                // XOR with dummy for side effect to prevent optimization
                for (size_t i = 0; i < this_batch_size; i++) {
                    dummy ^= batch[i];
                }
            }
            if (dummy == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy << std::endl;
            break;
        }
        
        case BitWidth::Bits64: {
            // 64-bit batch benchmark
            std::vector<uint64_t> batch(batch_size);
            uint64_t dummy = 0;
            
            for (size_t b = 0; b < batch_count; b++) {
                size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ? 
                                          iterations % batch_size : batch_size;
                
                // Generate batch
                for (size_t i = 0; i < this_batch_size; i++) {
                    batch[i] = rng();
                }
                
                // XOR with dummy for side effect to prevent optimization
                for (size_t i = 0; i < this_batch_size; i++) {
                    dummy ^= batch[i];
                }
            }
            if (dummy == 0xDEADBEEFDEADBEEF) std::cout << "Unlikely value: " << dummy << std::endl;
            break;
        }
        
        case BitWidth::Bits128: {
            // 128-bit batch benchmark
            size_t u64_count = batch_size * 2; // Each 128-bit number is 2 uint64_t
            std::vector<uint64_t> batch(u64_count);
            std::array<uint64_t, 2> dummy = {0};

            for (size_t b = 0; b < batch_count; b++) {
                size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ?
                                        iterations % batch_size : batch_size;

                // Generate batch
                for (size_t i = 0; i < this_batch_size; i++) {
                    for (int j = 0; j < 2; j++) {
                        batch[i*2 + j] = rng();
                    }
                }

                // XOR with dummy for side effect to prevent optimization
                for (size_t i = 0; i < this_batch_size; i++) {
                    for (int j = 0; j < 2; j++) {
                        dummy[j] ^= batch[i*2 + j];
                    }
                }
            }
            if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
            break;
        }

        case BitWidth::Bits256: {
            // 256-bit batch benchmark
            size_t u64_count = batch_size * 4; // Each 256-bit number is 4 uint64_t
            std::vector<uint64_t> batch(u64_count);
            std::array<uint64_t, 4> dummy = {0};
            
                for (size_t b = 0; b < batch_count; b++) {
                    size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ? 
                                              iterations % batch_size : batch_size;
                    
                    // Generate batch
                    for (size_t i = 0; i < this_batch_size; i++) {
                        for (int j = 0; j < 4; j++) {
                            batch[i*4 + j] = rng();
                        }
                    }
                    
                    // XOR with dummy for side effect to prevent optimization
                    for (size_t i = 0; i < this_batch_size; i++) {
                        for (int j = 0; j < 4; j++) {
                            dummy[j] ^= batch[i*4 + j];
                        }
                    }
                }
                if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
                break;
            }
            
            case BitWidth::Bits512: {
                // 512-bit batch benchmark
                size_t u64_count = batch_size * 8; // Each 512-bit number is 8 uint64_t
                std::vector<uint64_t> batch(u64_count);
                std::array<uint64_t, 8> dummy = {0};
                
                for (size_t b = 0; b < batch_count; b++) {
                    size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ? 
                                              iterations % batch_size : batch_size;
                    
                    // Generate batch
                    for (size_t i = 0; i < this_batch_size; i++) {
                        for (int j = 0; j < 8; j++) {
                            batch[i*8 + j] = rng();
                        }
                    }
                    
                    // XOR with dummy for side effect to prevent optimization
                    for (size_t i = 0; i < this_batch_size; i++) {
                        for (int j = 0; j < 8; j++) {
                            dummy[j] ^= batch[i*8 + j];
                        }
                    }
                }
                if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
                break;
            }
            
            case BitWidth::Bits1024: {
                // 1024-bit batch benchmark
                size_t u64_count = batch_size * 16; // Each 1024-bit number is 16 uint64_t
                std::vector<uint64_t> batch(u64_count);
                std::array<uint64_t, 16> dummy = {0};
                
                for (size_t b = 0; b < batch_count; b++) {
                    size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ? 
                                              iterations % batch_size : batch_size;
                    
                    // Generate batch
                    for (size_t i = 0; i < this_batch_size; i++) {
                        for (int j = 0; j < 16; j++) {
                            batch[i*16 + j] = rng();
                        }
                    }
                    
                    // XOR with dummy for side effect to prevent optimization
                    for (size_t i = 0; i < this_batch_size; i++) {
                        for (int j = 0; j < 16; j++) {
                            dummy[j] ^= batch[i*16 + j];
                        }
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
    
    // Benchmarking for Xoroshiro128+ (single mode)
    double benchmark_xoroshiro128plus_single(Xoroshiro128Plus& rng, BitWidth width, size_t iterations) {
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
            
            case BitWidth::Bits128: {
                // 128-bit benchmark
                uint64_t values[2] = {0};
                std::array<uint64_t, 2> dummy = {0};
    
                for (size_t i = 0; i < iterations; i++) {
                    rng.next_u128(values);
                    // XOR the values with our running total
                    for (int j = 0; j < 2; j++) {
                        dummy[j] ^= values[j];
                    }
                }
                if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely values: " << dummy[0] << std::endl;
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
    
    // Benchmarking for Xoroshiro128+ (batch mode)
    double benchmark_xoroshiro128plus_batch(Xoroshiro128Plus& rng, BitWidth width, size_t iterations) {
        auto start = std::chrono::high_resolution_clock::now();
        size_t batch_size = get_batch_size(width);
        size_t batch_count = (iterations + batch_size - 1) / batch_size;
    
        // Use different benchmarking approach based on bit width
        switch (width) {
            case BitWidth::Bits16: {
                // 16-bit batch benchmark
                std::vector<uint16_t> batch(batch_size);
                uint16_t dummy = 0;
                
                for (size_t b = 0; b < batch_count; b++) {
                    size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ? 
                                              iterations % batch_size : batch_size;
    
                    // Generate batch
                    rng.generate_batch_u16(batch.data(), this_batch_size);
    
                    // XOR with dummy for side effect to prevent optimization
                    for (size_t i = 0; i < this_batch_size; i++) {
                        dummy ^= batch[i];
                    }
                }
                if (dummy == 0xDEAD) std::cout << "Unlikely value: " << dummy << std::endl;
                break;
            } 
        
            case BitWidth::Bits32: {
                // 32-bit batch benchmark
                std::vector<uint32_t> batch(batch_size);
                uint32_t dummy = 0;
                
                for (size_t b = 0; b < batch_count; b++) {
                    size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ? 
                                              iterations % batch_size : batch_size;
    
                    // Generate batch
                    rng.generate_batch_u32(batch.data(), this_batch_size);
    
                    // XOR with dummy for side effect to prevent optimization
                    for (size_t i = 0; i < this_batch_size; i++) {
                        dummy ^= batch[i];
                    }
                }
                if (dummy == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy << std::endl;
                break;
            }
        
            case BitWidth::Bits64: {
                // 64-bit batch benchmark
                std::vector<uint64_t> batch(batch_size);
                uint64_t dummy = 0;
                
                for (size_t b = 0; b < batch_count; b++) {
                    size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ? 
                                              iterations % batch_size : batch_size;
    
                    // Generate batch
                    rng.generate_batch_u64(batch.data(), this_batch_size);
    
                    // XOR with dummy for side effect to prevent optimization
                    for (size_t i = 0; i < this_batch_size; i++) {
                        dummy ^= batch[i];
                    }
                }
                if (dummy == 0xDEADBEEFDEADBEEF) std::cout << "Unlikely value: " << dummy << std::endl;
                break;
            }
    
            case BitWidth::Bits128: {
                // 128-bit batch benchmark
                size_t u64_count = batch_size * 2; // Each 128-bit number is 2 uint64_t
                std::vector<uint64_t> batch(u64_count);
                std::array<uint64_t, 2> dummy = {0};
    
                for (size_t b = 0; b < batch_count; b++) {
                    size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ?
                                            iterations % batch_size : batch_size;
    
                    // Generate batch
                    rng.generate_batch_u128(batch.data(), this_batch_size);
    
                    // XOR with dummy for side effects to prevent optimization
                    for (size_t i = 0; i < this_batch_size; i++) {
                       for (int j = 0; j < 2; j++) {
                            dummy[j] ^= batch[i*2 + j];
                       }
                    }
                }
                if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely values: " << dummy[0] << std::endl;
                break;
            }
    
            case BitWidth::Bits256: {
                // 256-bit batch benchmark
                size_t u64_count = batch_size * 4; // Each 256-bit number is 4 uint64_t
                std::vector<uint64_t> batch(u64_count);
                std::array<uint64_t, 4> dummy = {0};
                
                for (size_t b = 0; b < batch_count; b++) {
                    size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ? 
                                              iterations % batch_size : batch_size;
    
                    // Generate batch
                    rng.generate_batch_u256(batch.data(), this_batch_size);
    
                    // XOR with dummy for side effect to prevent optimization
                    for (size_t i = 0; i < this_batch_size; i++) {
                        for (int j = 0; j < 4; j++) {
                            dummy[j] ^= batch[i*4 + j];
                        }
                    }
                }
                if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
                break;
            }
        
            case BitWidth::Bits512: {
                // 512-bit batch benchmark
                size_t u64_count = batch_size * 8; // Each 512-bit number is 8 uint64_t
                std::vector<uint64_t> batch(u64_count);
                std::array<uint64_t, 8> dummy = {0};
                
                for (size_t b = 0; b < batch_count; b++) {
                    size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ? 
                                              iterations % batch_size : batch_size;
    
                    // Generate batch
                    rng.generate_batch_u512(batch.data(), this_batch_size);
    
                    // XOR with dummy for side effect to prevent optimization
                    for (size_t i = 0; i < this_batch_size; i++) {
                        for (int j = 0; j < 8; j++) {
                            dummy[j] ^= batch[i*8 + j];
                        }
                    }
                }    
                if (dummy[0] == 0xDEADBEEF) std::cout << "Unlikely value: " << dummy[0] << std::endl;
                break;
            }
        
            case BitWidth::Bits1024: {
                // 1024-bit batch benchmark
                size_t u64_count = batch_size * 16; // Each 1024-bit number is 16 uint64_t
                std::vector<uint64_t> batch(u64_count);
                std::array<uint64_t, 16> dummy = {0};
                
                for (size_t b = 0; b < batch_count; b++) {
                    size_t this_batch_size = (b == batch_count - 1 && iterations % batch_size != 0) ? 
                                              iterations % batch_size : batch_size;
    
                    // Generate batch
                    rng.generate_batch_u1024(batch.data(), this_batch_size);
    
                    // XOR with dummy for side effect to prevent optimization
                    for (size_t i = 0; i < this_batch_size; i++) {
                        for (int j = 0; j < 16; j++) {
                            dummy[j] ^= batch[i*16 + j];
                        }
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
    
    int main(int argc, char* argv[]) {
        std::cout << "Enhanced Bit Width Benchmark\n";
        std::cout << "==========================\n\n";
    
        // Parse command line arguments
        size_t iterations = 10000000;  // Default: 10 million
        BitWidth selected_width = BitWidth::Bits64;
    
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
                      << "4. 128-bit\n"               
                      << "5. 256-bit\n"
                      << "6. 512-bit\n"
                      << "7. 1024-bit\n"
                      << "Enter choice (1-7): ";
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
    
        // Benchmark Xoroshiro128++ (single)
        universal_rng_t* xoroshiro_rng = universal_rng_new(SEED, 0, 1); // 0 = Xoroshiro, 1 = Double precision
        if (!xoroshiro_rng) {
            std::cerr << "Failed to create Xoroshiro RNG!" << std::endl;
            return 1;
        }
    
        const char* xoroshiro_impl = universal_rng_get_implementation(xoroshiro_rng);
        std::cout << "Benchmarking " << xoroshiro_impl << " (Single Mode)" << std::endl;
        double xoroshiro_single_time = benchmark_universal_rng_single(xoroshiro_rng, selected_width, iterations);
        double xoroshiro_single_rate = iterations / xoroshiro_single_time / 1'000'000;
        results.push_back({xoroshiro_impl, "Single", bitwidth_to_string(selected_width), xoroshiro_single_time, xoroshiro_single_rate});
    
        // Benchmark Xoroshiro128++ (batch mode)
        std::cout << "Benchmarking " << xoroshiro_impl << " (Batch Mode)" << std::endl;
        double xoroshiro_batch_time = benchmark_universal_rng_batch(xoroshiro_rng, selected_width, iterations);
        double xoroshiro_batch_rate = iterations / xoroshiro_batch_time / 1'000'000;
        results.push_back({xoroshiro_impl, "Batch", bitwidth_to_string(selected_width), xoroshiro_batch_time, xoroshiro_batch_rate});   
    
        // Benchmark WyRand (single)
        universal_rng_t* wyrand_rng = universal_rng_new(SEED, 1, 1); // 1 = WyRand, 1 = Double precision
        if (!wyrand_rng) {
            std::cerr << "Failed to create WyRand RNG!" << std::endl;
            universal_rng_free(xoroshiro_rng);
            universal_rng_free_string(xoroshiro_impl);
            return 1;
        }   
    
        const char* wyrand_impl = universal_rng_get_implementation(wyrand_rng);
        std::cout << "Benchmarking " << wyrand_impl << " (Single Mode)" << std::endl;
        double wyrand_single_time = benchmark_universal_rng_single(wyrand_rng, selected_width, iterations);
        double wyrand_single_rate = iterations / wyrand_single_time / 1'000'000;
        results.push_back({wyrand_impl, "Single", bitwidth_to_string(selected_width), wyrand_single_time, wyrand_single_rate}); 
    
        // Benchmark WyRand (batch)
        std::cout << "Benchmarking " << wyrand_impl << " (Batch Mode)" << std::endl;
        double wyrand_batch_time = benchmark_universal_rng_batch(wyrand_rng, selected_width, iterations);
        double wyrand_batch_rate = iterations / wyrand_batch_time / 1'000'000;
        results.push_back({wyrand_impl, "Batch", bitwidth_to_string(selected_width), wyrand_batch_time, wyrand_batch_rate});    
    
        // Benchmark std::mt19937_64 (single)
        std::mt19937_64 mt_rng(SEED);
        std::cout << "Benchmarking std::mt19937_64 (Single Mode)" << std::endl;
        double mt_single_time = benchmark_std_mt_single(mt_rng, selected_width, iterations);
        double mt_single_rate = iterations / mt_single_time / 1'000'000;
        results.push_back({"std::mt19937_64", "Single", bitwidth_to_string(selected_width), mt_single_time, mt_single_rate});   
    
        // Benchmark std::mt19937_64 (batch)
        std::cout << "Benchmarking std::mt19937_64 (Batch Mode)" << std::endl;
        double mt_batch_time = benchmark_std_mt_batch(mt_rng, selected_width, iterations);
        double mt_batch_rate = iterations / mt_batch_time / 1'000'000;
        results.push_back({"std::mt19937_64", "Batch", bitwidth_to_string(selected_width), mt_batch_time, mt_batch_rate});  
    
        // Benchmark basic Xoroshiro128+ (single)
        Xoroshiro128Plus xoro_plus(SEED);
        std::cout << "Benchmarking Xoroshiro128+ (Single Mode)" << std::endl;
        double xoro_plus_single_time = benchmark_xoroshiro128plus_single(xoro_plus, selected_width, iterations);
        double xoro_plus_single_rate = iterations / xoro_plus_single_time / 1'000'000;
        results.push_back({"Xoroshiro128+", "Single", bitwidth_to_string(selected_width), xoro_plus_single_time, xoro_plus_single_rate});   
    
        // Benchmark basic Xoroshiro128+ (batch)
        std::cout << "Benchmarking Xoroshiro128+ (Batch Mode)" << std::endl;
        double xoro_plus_batch_time = benchmark_xoroshiro128plus_batch(xoro_plus, selected_width, iterations);
        double xoro_plus_batch_rate = iterations / xoro_plus_batch_time / 1'000'000;
        results.push_back({"Xoroshiro128+", "Batch", bitwidth_to_string(selected_width), xoro_plus_batch_time, xoro_plus_batch_rate});  
    
        // Print results table
        std::cout << std::endl;
        std::cout << "Benchmark Results for " << bitwidth_to_string(selected_width) << std::endl;
        std::cout << "================================" << std::endl;
        std::cout << std::left << std::setw(25) << "Implementation" 
              << std::setw(10) << "Mode"
              << std::setw(15) << "Time (sec)" 
              << std::setw(15) << "Speed (M/s)" << std::endl;
        std::cout << std::string(65, '-') << std::endl; 
    
        for (const auto& result : results) {
            std::cout << std::left << std::setw(25) << result.name
                  << std::setw(10) << result.mode
                  << std::fixed << std::setprecision(4) << std::setw(15) << result.time
                  << std::fixed << std::setprecision(2) << std::setw(15) << result.rate << std::endl;
        }       
    
        // Find fastest implementation
        auto fastest = *std::max_element(results.begin(), results.end(),
            [](const BenchResult& a, const BenchResult& b) {
                return a.rate < b.rate;
            }); 
    
        std::cout << "\nFastest implementation: " << fastest.name << " (" << fastest.mode << " Mode) at " 
              << std::fixed << std::setprecision(2) << fastest.rate << " M/s" << std::endl; 
    
        // Speedup analysis
        std::cout << std::endl;
        std::cout << "Batch vs Single Mode Speedup:" << std::endl;
        std::cout << "============================" << std::endl;
        std::cout << xoroshiro_impl << ": " << std::fixed << std::setprecision(2) 
              << xoroshiro_single_time / xoroshiro_batch_time << "x faster in batch mode" << std::endl;
        std::cout << wyrand_impl << ": " << std::fixed << std::setprecision(2) 
              << wyrand_single_time / wyrand_batch_time << "x faster in batch mode" << std::endl;
        std::cout << "std::mt19937_64: " << std::fixed << std::setprecision(2) 
              << mt_single_time / mt_batch_time << "x faster in batch mode" << std::endl;
        std::cout << "Xoroshiro128+: " << std::fixed << std::setprecision(2) 
              << xoro_plus_single_time / xoro_plus_batch_time << "x faster in batch mode" << std::endl; 
    
        // Comparative analysis between implementations (single mode)
        std::cout << std::endl;
        std::cout << "Single Mode Comparative Analysis:" << std::endl;
        std::cout << "================================" << std::endl;
        std::cout << "Compared to std::mt19937_64:" << std::endl;
        std::cout << "  " << xoroshiro_impl << ": " << std::fixed << std::setprecision(2) 
              << mt_single_time / xoroshiro_single_time << "x faster" << std::endl;
        std::cout << "  " << wyrand_impl << ": " << std::fixed << std::setprecision(2) 
              << mt_single_time / wyrand_single_time << "x faster" << std::endl;
        std::cout << "  Xoroshiro128+: " << std::fixed << std::setprecision(2) 
              << mt_single_time / xoro_plus_single_time << "x faster" << std::endl; 
    
        // Comparative analysis between implementations (batch mode)
        std::cout << std::endl;
        std::cout << "Batch Mode Comparative Analysis:" << std::endl;
        std::cout << "================================" << std::endl;
        std::cout << "Compared to std::mt19937_64:" << std::endl;
        std::cout << "  " << xoroshiro_impl << ": " << std::fixed << std::setprecision(2) 
              << mt_batch_time / xoroshiro_batch_time << "x faster" << std::endl;
        std::cout << "  " << wyrand_impl << ": " << std::fixed << std::setprecision(2) 
              << mt_batch_time / wyrand_batch_time << "x faster" << std::endl;
        std::cout << "  Xoroshiro128+: " << std::fixed << std::setprecision(2) 
              << mt_batch_time / xoro_plus_batch_time << "x faster" << std::endl;
        
    // Cleanup
    universal_rng_free(xoroshiro_rng);
    universal_rng_free(wyrand_rng);
    universal_rng_free_string(xoroshiro_impl);
    universal_rng_free_string(wyrand_impl);

    std::cout << "\nBenchmark completed successfully!" << std::endl;
    return 0;
}