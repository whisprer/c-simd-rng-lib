# Universal Architecture PRNG std. Replacement C++ Lib

[README.md]

# C-Simd-Rng-Lib

<p align="center">
  <a href="https://github.com/whisprer/c-simd-rng-lib/releases"> 
    <img src="https://img.shields.io/github/v/release/whisprer/c-simd-rng-lib?color=4CAF50&label=release" alt="Release Version"> 
  </a>
  <a href="https://github.com/whisprer/c-simd-rng-lib/actions"> 
    <img src="https://img.shields.io/github/actions/workflow/status/whisprer/c-simd-rng-lib/lint-and-plot.yml?label=build" alt="Build Status"> 
  </a>
</p>

![Commits](https://img.shields.io/github/commit-activity/m/whisprer/c-simd-rng-lib?label=commits) 
![Last Commit](https://img.shields.io/github/last-commit/whisprer/c-simd-rng-lib) 
![Issues](https://img.shields.io/github/issues/whisprer/c-simd-rng-lib) 
[![Version](https://img.shields.io/badge/version-3.1.1-blue.svg)](https://github.com/whisprer/c-simd-rng-lib) 
[![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-lightgrey.svg)](https://www.microsoft.com/windows)
[![Python](https://img.shields.io/badge/python-3.8%2B-blue.svg)](https://www.python.org)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

<p align="center">
  <img src="c-simd-rng-lib-banner.png" width="850" alt="C-Simd-Rng-Lib Banner"> 
</p>

**A high-performance, cross-platform random number generation library with SIMD and GPU acceleration.**

## Overview - [resuced from eird formatting doldrums by the one and only RTC!!! thanxyou :) ]

`universal_rng_lib` is a fast, flexible RNG library written in modern C++. It supports a range of algorithms including **Xoroshiro128++** and **WyRand**, with runtime autodetection of the best CPU vectorization (SSE2, AVX2, AVX-512, NEON) and optional OpenCL GPU support.

It significantly outperforms the C++ standard library RNGs and can replace them in scientific simulations, games, real-time systems, and more.

## Features

- ✅ Multiple PRNGs: `Xoroshiro128++`, `WyRand`
- ✅ SIMD Acceleration: SSE2, AVX2, AVX-512, NEON (auto-detect at runtime)
- ✅ OpenCL GPU support (optional)
- ✅ Scalar fallback for universal compatibility
- ✅ Batch generation for improved throughput
- ✅ Support for 16–1024 bit generation
- ✅ Cross-platform: Windows (MSVC, MinGW), Linux
- ✅ MIT Licensed

## Quick Start

### Requirements

- C++17-compatible compiler
- CMake 3.15+
- Ninja (recommended)
- OpenCL SDK (optional)

### Build Instructions

#### Linux/Mac (bash)
```bash
git clone https://github.com/YOUR_USERNAME/universal_rng_lib.git
cd universal_rng_lib
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
./rng_selftest
```

#### Windows (MSYS2 MinGW64 shell)
```bash
git clone https://github.com/YOUR_USERNAME/universal_rng_lib.git
cd universal_rng_lib
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DRNG_WITH_OPENCL=OFF
cmake --build . --parallel
./rng_selftest.exe
```

> **Note:** If AVX2 is supported, it will automatically be compiled in and used.

### Usage Example

```cpp
#include "universal_rng.h"
#include <iostream>

int main() {
    // Create RNG instance (seed, algorithm_id, bitwidth)
    universal_rng_t* rng = universal_rng_new(1337, 0, 1);

    // Generate 64-bit random integer
    uint64_t val = universal_rng_next_u64(rng);
    std::cout << "Random u64: " << val << std::endl;

    // Generate double in range [0,1)
    double d = universal_rng_next_double(rng);
    std::cout << "Random double: " << d << std::endl;

    // Cleanup
    universal_rng_free(rng);
}
```

### Replace C++ Standard RNG

To use `universal_rng_lib` in place of `std::mt19937` or `std::default_random_engine`:

**Replace all instances of:**
```cpp
std::mt19937 rng(seed);
```

**with:**
```cpp
auto* rng = universal_rng_new(seed, 0, 1);  // use algorithm 0 = Xoroshiro128++
```

**Replace:**
```cpp
rng(); // or dist(rng)
```

**with:**
```cpp
universal_rng_next_u64(rng);
```

Use `universal_rng_next_double(rng);` for floating-point needs.

**Replace cleanup:**
```cpp
delete rng;
```

**with:**
```cpp
universal_rng_free(rng);
```

## File Structure

```
.
├── include/                # All public headers
│   └── universal_rng.h    # Main header
├── Benchmarking/           # Benchmarking Results 
│                           # [compared against C++ std. lib]
├── src/                    # Source code
│   ├── simd_avx2.cpp
│   ├── simd_sse2.cpp
│   ├── simd_avx512.cpp
│   ├── universal_rng.cpp
│   └── runtime_detect.cpp
├── lib_files/              # Prebuilt binaries
│   ├── mingw_shared/
│   ├── msvc_shared/
│   └── linux_shared/
├── extras/                 # Environment setups and tools
│   └── windows/
├── docs/                   # In-depth design documentation
│   ├── key_SIMD-implementation_design-principles.md
│   ├── explain_of_3-7's_refactor.md
│   └── opencl-implementation-details.md
└── tests/                  # Self-test and benchmarks
```

## SIMD & Dispatch Design

- Auto-detects best available instruction set at runtime
- Gracefully falls back to scalar or SSE2
- Batches can be used to further accelerate performance
- Detection failures are handled gracefully

**Example detection result:**
```yaml
CPU feature detection:
  SSE2: Yes
  AVX2: Yes
  AVX512: No
Trying AVX2 implementation...
Using AVX2 implementation
```

## Benchmarking & Performance

- Batch mode yields **1.7×–2.5×** speedup over naive generation
- AVX2 performs **~3–5×** faster than `std::mt19937`
- AVX-512 versions under development

## License

MIT License – see [LICENSE.md](LICENSE.md) for full terms.

## Reference

This library is partially inspired by:
- David Blackman & Sebastiano Vigna's paper on Scrambled Linear PRNGs (SLRNG)
