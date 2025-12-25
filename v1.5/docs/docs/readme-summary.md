Universal High-Performance RNG Library (C++)

Project Overview

This ambitious project aims to create a highly optimized, dynamically adaptive Random Number Generator (RNG) library in C++, significantly outperforming standard C++ RNG implementations across various hardware architectures. The library supports multiple RNG algorithms, notably Xoroshiro128++ and WyRand, intelligently exploiting the optimal SIMD (Scalar, SSE2, AVX2, AVX512, NEON) and GPU (OpenCL) implementations at runtime.

Core Goals & Priorities

Performance (Highest Priority)

Primary goal: surpass C++ standard RNG implementations, particularly in generation speed and batch throughput.

Randomness Quality

Using Xoroshiro128++ and WyRand, both adequate for scientific and gaming applications.

WyRand noted for slightly superior randomness quality.

Memory Efficiency

Generally modest memory usage; not a major issue but monitored for extreme scenarios.

Technical Architecture and Features

Runtime SIMD Detection

Runtime detection and adaptive selection of optimal SIMD implementation (Scalar, SSE2, AVX2, AVX512, NEON).

Dispatcher dynamically selects the optimal implementation based on CPU feature detection, ensuring optimal performance across platforms.

Implementation Details

Core Components

Headers and Types:

universal_rng.h, universal_rng_types.h, universal_rng_config.h.

Scalar and SIMD Implementations:

Scalar fallback implementation.

SIMD versions: SSE2 (2-way), AVX2 (4-way), AVX512 (8-way).

State Management:

Robust initialization using SplitMix64 for state management.

SIMD Optimization

Scalar Fallback:

Ensures compatibility.

SSE2 Implementation:

2-way parallelism; compensates for missing native 64-bit rotations.

AVX2 Implementation:

4-way parallelism; efficient vectorized operations and batch pre-generation.

AVX512 Implementation:

8-way parallelism with advanced optimization, supporting multiple AVX-512 extensions (AVX-512F, AVX-512DQ, AVX-512BW, AVX-512VL).

GPU Acceleration (OpenCL)

Robust integration prioritizing NVIDIA, Intel, AMD GPUs.

Batch generation significantly increasing throughput.

Benchmarking Framework

universal_bench.cpp and rng_bench_comprehensive.cpp:

Provide detailed benchmarks, performance metrics (min, max, average, variance).

Structured comparison between SIMD implementations, scalar fallback, and GPU acceleration.

Modern C++ Refactoring

Smart Pointers:

Replaced raw pointers with std::unique_ptr and std::shared_ptr.

Modern C++ Features:

std::array, RAII wrappers, move semantics, rule-of-five, constexpr, and namespaces.

Safer casting: static_cast, reinterpret_cast.

Enhanced Benchmarking:

Precise timing using std::chrono.

Usage Instructions (Compiler Directives)

Build options automatically detect SIMD capabilities and GPU support:

compile                # Default universal binary with runtime detection
compile avx512f avx512dq avx512bw avx512vl - optimized for specific SIMD sets
compile no-opencl     - excludes OpenCL support
compile avx2          - explicitly compiles AVX2 variant

Current Performance Observations

Batch mode: Significant speedup (1.40x) over one-at-a-time approach.

Universal RNG implementation currently slightly slower (0.68x) than original Xoroshiro128+ in non-batch mode; optimization ongoing.

Next Steps & Recommendations

Optimization:

Resolve erroneous AVX-512 detection causing suboptimal dispatching.

Minimize dispatch overhead in universal RNG.

Optimize AVX-512 implementations for better register and cache utilization.

Documentation Completion:

Fully document implementation details, including OpenCL kernels and CPU feature detection logic.

Cross-platform Support:

Port Windows batch scripts to Linux/macOS shell scripts.

Validate memory alignment consistency across platforms.

Technical Observations & Final Thoughts

This RNG project demonstrates impressive maturity, modularity, and optimization:

Adaptive runtime detection ensures flexibility and performance.

Comprehensive SIMD and GPU acceleration strategies are sophisticated and robust.

Extensive and structured benchmarking allows precise performance comparisons.

Potential Enhancements

Implement a comprehensive build system (e.g., CMake) for cross-platform support.

Consistent application of verbose logging for easier debugging.

Continuous checks for memory alignment, especially for SIMD buffers.