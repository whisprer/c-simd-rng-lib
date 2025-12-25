Modern RNG Implementation
This project provides modernized C++ implementations of fast random number generators:

WyRand
Xoroshiro128++

The implementations use modern C++ features such as smart pointers, RAII, namespaces, and template-based generic programming while maintaining high performance and backward compatibility with existing C APIs.
Features

Modern C++ Design: Uses C++17 features including smart pointers, std::array, auto, constexpr, etc.
RAII: Proper resource management with destructors and move semantics
Better Memory Management: Uses std::unique_ptr and std::shared_ptr instead of raw pointers
Namespace Organization: Code organized into namespaces
Template-Based Benchmarking: Generic benchmarking framework
Precise Timing: Uses std::chrono for more accurate performance measurements
SIMD Optimization: Automatically detects and utilizes available SIMD instructions
Backward Compatibility: Maintains C API for existing code

Getting Started
Build Requirements

C++17 compliant compiler (GCC 7+, Clang 5+, MSVC 2017+)
CMake 3.10 or newer

Building the Project
bashCopymkdir build
cd build
cmake ..
make
Running the Benchmarks
After building, you can run the benchmarks:
bashCopy# Run WyRand benchmark
./wyrand_bench

# Run Xoroshiro128++ benchmark
./xoroshiro_bench

# Run comprehensive benchmark of all RNG implementations
./rng_comprehensive_bench

# You can also specify the number of iterations:
./rng_comprehensive_bench 500000000  # 500 million iterations
Usage Examples
Using WyRand RNG
cppCopy#include <rng/wyrand.h>
#include <iostream>

int main() {
    // Create a WyRand RNG with a seed
    rng::WyRand rng(12345);
    
    // Generate random integers
    for (int i = 0; i < 5; i++) {
        std::cout << rng.next_u64() << std::endl;
    }
    
    // Generate random doubles in [0,1)
    for (int i = 0; i < 5; i++) {
        std::cout << rng.next_double() << std::endl;
    }
    
    return 0;
}
Using Xoroshiro128++
cppCopy#include <rng/xoroshiro128pp.h>
#include <iostream>

int main() {
    // Create a Xoroshiro128++ RNG with a seed
    rng::Xoroshiro128PlusPlus rng(12345);
    
    // Generate random integers
    for (int i = 0; i < 5; i++) {
        std::cout << rng.next_u64() << std::endl;
    }
    
    // Generate random doubles in [0,1)
    for (int i = 0; i < 5; i++) {
        std::cout << rng.next_double() << std::endl;
    }
    
    // Jump ahead (equivalent to 2^64 calls to next_u64())
    rng.jump();
    
    // Generate more random numbers after the jump
    std::cout << "After jump: " << rng.next_u64() << std::endl;
    
    return 0;
}
Using C API (Backward Compatibility)
cCopy#include <rng/wyrand_simd_main_optimized.h>
#include <stdio.h>

int main() {
    // Create a WyRand RNG with a seed
    wyrand_simd_rng* rng = wyrand_simd_new(12345);
    
    // Generate random integers
    for (int i = 0; i < 5; i++) {
        printf("%llu\n", (unsigned long long)wyrand_simd_next_u64(rng));
    }
    
    // Generate random doubles in [0,1)
    for (int i = 0; i < 5; i++) {
        printf("%f\n", wyrand_simd_next_double(rng));
    }
    
    // Clean up
    wyrand_simd_free(rng);
    
    return 0;
}
Performance
The modernized implementations maintain the high performance of the original algorithms while adding safety and maintainability. The benchmarks measure the generation speed in millions of numbers per second.
Typical performance on modern hardware:

Xoroshiro128+ (original): Base reference
Xoroshiro128++ (optimized): Up to 1.5x faster than the original
WyRand (optimized): Up to 2.0x faster than Xoroshiro128+

Actual performance will vary based on hardware, compiler, and available SIMD instructions.
License
This project is open source and available under the MIT License.