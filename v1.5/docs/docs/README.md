# universal_rng_lib
 A high-performance RNG library with SIMD and GPU acceleration


# Universal High-Performance RNG Library

A highly optimized random number generator library with SIMD and GPU acceleration support, significantly outperforming standard C++ RNG implementations.

## Features

- Multiple algorithms: Xoroshiro128++ and WyRand
- Runtime detection of optimal SIMD implementation (AVX-512, AVX2, SSE2, NEON)
- OpenCL support for GPU acceleration
- Batch generation for improved performance
- Support for various bit widths (16-bit to 1024-bit)
- Comprehensive benchmarking suite

## Getting Started

### Requirements
- C++17 compatible compiler
- OpenCL libraries (optional, for GPU acceleration)

### Building
```bash
# Clone the repository
git clone https://github.com/YOUR_USERNAME/YOUR_REPOSITORY_NAME.git
cd YOUR_REPOSITORY_NAME

# Build using provided batch script
simple_compiler.bat


Usage Example
cppCopy#include "universal_rng.h"
#include <iostream>

int main() {
    // Create an RNG with Xoroshiro128++ algorithm and double precision
    universal_rng_t* rng = universal_rng_new(42, 0, 1);
    
    // Generate a random 64-bit number
    uint64_t value = universal_rng_next_u64(rng);
    std::cout << "Random number: " << value << std::endl;
    
    // Generate a random double in [0,1)
    double d = universal_rng_next_double(rng);
    std::cout << "Random double: " << d << std::endl;
    
    // Clean up
    universal_rng_free(rng);
    
    return 0;
}


Universal High-Performance RNG Library

A highly optimized, dynamically adaptive random number generator (RNG) library in C++ with SIMD and GPU acceleration support, significantly outperforming standard C++ RNG implementations across various hardware architectures.


Project Overview

This ambitious project provides multiple RNG algorithms, notably Xoroshiro128++ and WyRand, and intelligently exploits optimal SIMD (Scalar, SSE2, AVX2, AVX-512, NEON) and GPU (OpenCL) implementations at runtime.


Core Goals & Priorities

Performance (Highest Priority)

Aim: Surpass C++ standard RNG implementations in generation speed and batch throughput.


Randomness Quality

Algorithms: Xoroshiro128++ and WyRand.

WyRand noted for slightly superior randomness quality, suitable for scientific and gaming applications.


Memory Efficiency

Modest memory usage, monitored particularly for extreme scenarios.


Technical Architecture & Features

Runtime SIMD Detection

Dynamically selects optimal SIMD implementation based on CPU features.

Supported implementations: Scalar (fallback), SSE2 (2-way), AVX2 (4-way), AVX-512 (8-way), and NEON.

GPU Acceleration (OpenCL)

Supports NVIDIA, Intel, AMD GPUs.

Batch generation significantly increases throughput.


Implementation Details

Core Components

Headers and Types:

`universal_rng.h`


Scalar and SIMD Implementations

Scalar: Ensures compatibility as fallback.

SSE2: 2-way parallelism; compensates for missing native 64-bit rotations.

AVX2: 4-way parallelism with efficient vectorized operations.

AVX-512: 8-way parallelism; supports multiple AVX-512 extensions (AVX-512F, AVX-512DQ, AVX-512BW, AVX-512VL).


State Management

Robust initialization with SplitMix64.

Modern C++ Refactoring

Smart pointers (std::unique_ptr, std::shared_ptr).

Modern features: std::array, RAII, move semantics, rule-of-five, constexpr, namespaces.

Safer casting: static_cast, reinterpret_cast.


Benchmarking Framework

Provides detailed performance metrics (min, max, average, variance).



Getting Started


Requirements

C++17-compatible compiler

OpenCL libraries (optional, for GPU acceleration)


Building

Clone and build using provided scripts:

git clone https://github.com/YOUR_USERNAME/YOUR_REPOSITORY_NAME.git
cd YOUR_REPOSITORY_NAME

# Default build (runtime detection)
compile

# Specific SIMD optimization
compile avx512f avx512dq avx512bw avx512vl

# Exclude OpenCL support
compile no-opencl

# Explicit AVX2 compilation
compile avx2


Usage Example

#include "universal_rng.h"
#include <iostream>

int main() {
    auto rng = universal_rng_new(42, 0, 1);

    uint64_t value = universal_rng_next_u64(rng);
    std::cout << "Random number: " << value << std::endl;

    double d = universal_rng_next_double(rng);
    std::cout << "Random double: " << d << std::endl;

    universal_rng_free(rng);

    return 0;
}


Current Performance Observations

Batch mode: Significant speedup (1.??x-2.??x) over single-generation approach.

Current universal RNG slightly slower (0.3x-0.7x) than Xoroshiro128+ in non-batch mode; optimization ongoing.

Next Steps & Recommendations

Optimization:

Resolve erroneous AVX-512 detection causing suboptimal dispatch.

Minimize dispatch overhead.

Optimize AVX-512 implementations.


Documentation:

Complete OpenCL kernel documentation.

Detail CPU feature detection logic.

Cross-platform Support:

Port Windows batch scripts to Linux/macOS shell scripts.

Ensure consistent memory alignment.


File Structure

universal_rng_lib/
├── .git/
├── build/
├── docs/
│   ├── c [old]/
│   ├── rs/
│   ├── avx-512_optimizations_done.md
│   ├── c++_rng_-_original_idea.md
│   ├── desired.md
│   ├── explain_of_3-7's_refactor.md
│   ├── key_SIMD_implementation_design_principles.md
│   ├── opencl_implementation_details.md
│   ├── README.md
│   ├── ScrambledLinear.pdf
│   ├── todo_avx2_optimizations.md
│   ├── todo_general_optimizations.md
│   └── why_api_and_main_stub_are_so_-_complicated.md
├── rs_port/
├── src/
│   └── c/
│       └── old_junk_files/
│           ├── dump/
│           ├── FINISHED/
│           ├── near_wworking_perfecc/
│           ├── previous/
│           ├── refactor_adv-bench_slim_512-dis/
│           ├── refactor_near_working/
│           ├── refactor+bench_512-dis/
│           ├── refactorr_working_no-bench_512-/
│           ├── temp/
│           └── THIS_VER_GOOD/
└── temp/


Technical Observations & Final Thoughts

This RNG library exhibits impressive maturity and optimization, demonstrating robust and flexible SIMD and GPU acceleration capabilities, backed by comprehensive benchmarking. Continued refinement and cross-platform compatibility are strongly recommended.


License

MIT License.