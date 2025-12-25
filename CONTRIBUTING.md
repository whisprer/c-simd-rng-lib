\# Contributing to Universal RNG Library



Thank you for your interest in contributing to the Universal RNG Library! This document provides guidelines for contributing code, reporting issues, and collaborating on this high-performance random number generation project.



\## Table of Contents

\- \[Code of Conduct](#code-of-conduct)

\- \[Getting Started](#getting-started)

\- \[Development Setup](#development-setup)

\- \[Contributing Code](#contributing-code)

\- \[Performance Standards](#performance-standards)

\- \[Testing Requirements](#testing-requirements)

\- \[Documentation](#documentation)

\- \[Issue Reporting](#issue-reporting)



---



\## Code of Conduct



This project values:

\- \*\*Technical excellence\*\* and performance-focused development

\- \*\*Cross-platform compatibility\*\* and inclusive hardware support

\- \*\*Clear communication\*\* about complex optimization topics

\- \*\*Constructive collaboration\*\* on SIMD and algorithmic improvements



---



\## Getting Started



\### Prerequisites

\- \*\*C++17\*\* compatible compiler (GCC 7+, Clang 5+, MSVC 2019+)

\- \*\*CMake 3.14+\*\* for build system management

\- \*\*Git\*\* for version control

\- Platform-specific tools:

&nbsp; - \*\*Windows:\*\* Visual Studio 2019+, MinGW64, or MSYS2

&nbsp; - \*\*Linux:\*\* GCC/Clang with SIMD intrinsics support

&nbsp; - \*\*macOS:\*\* Xcode command line tools



\### Development Environment Setup

```bash

\# Clone the repository

git clone \[repository-url]

cd universal-rng



\# Create development build

mkdir build-dev

cd build-dev

cmake -S .. -B . -DCMAKE\_BUILD\_TYPE=Debug

cmake --build .



\# Run tests

./rng\_selftest

```



---



\## Development Setup



\### Platform-Specific Notes



\#### Windows Development

\- \*\*MSVC:\*\* Use `/arch:AVX2` flags for SIMD optimization

\- \*\*MinGW64:\*\* Use `-mavx2 -march=native` for best performance

\- \*\*Shared Libraries:\*\* Ensure proper DLL export decoration



\#### Linux Development

\- \*\*GCC/Clang:\*\* Use `-march=native -O3` for optimization

\- \*\*SIMD Detection:\*\* Verify runtime vs compile-time feature detection

\- \*\*Shared Libraries:\*\* Use `-fPIC` for position-independent code



\### Required Tools by Platform

```bash

\# Windows (MSYS2/MinGW64)

pacman -S mingw-w64-x86\_64-toolchain mingw-w64-x86\_64-cmake mingw-w64-x86\_64-ninja



\# Ubuntu/Debian

sudo apt install build-essential cmake ninja-build



\# macOS

xcode-select --install

brew install cmake ninja

```



---



\## Contributing Code



\### Branch Strategy

\- \*\*main:\*\* Stable, tested code only

\- \*\*develop:\*\* Integration branch for new features

\- \*\*feature/xxx:\*\* Individual feature development

\- \*\*hotfix/xxx:\*\* Critical bug fixes

\- \*\*perf/xxx:\*\* Performance optimization branches



\### Coding Standards



\#### C++ Style Guidelines

\- \*\*Modern C++17\*\* features preferred

\- \*\*RAII\*\* for all resource management

\- \*\*Smart pointers\*\* over raw pointers

\- \*\*Template metaprogramming\*\* for compile-time optimization

\- \*\*Consistent naming:\*\* `snake\_case` for functions, `PascalCase` for classes



\#### SIMD Implementation Standards

```cpp

// Template for new SIMD implementations

class AlgorithmNameSIMD {

public:

&nbsp;   explicit AlgorithmNameSIMD(uint64\_t seed);

&nbsp;   void generate\_batch(uint64\_t\* dest, size\_t count);

&nbsp;   std::string get\_implementation\_name() const;

&nbsp;   

private:

&nbsp;   // SIMD-specific state

&nbsp;   // Aligned memory declarations

&nbsp;   // Helper functions

};

```



\#### Performance Expectations

\- \*\*Memory alignment:\*\* 64-byte alignment for AVX-512, 32-byte for AVX2

\- \*\*Branch elimination:\*\* Minimize conditional logic in hot paths

\- \*\*Cache efficiency:\*\* Optimize memory access patterns

\- \*\*SIMD utilization:\*\* Achieve theoretical speedup ratios



\### Commit Message Format

```

type(scope): brief description



Detailed explanation of changes, including:

\- Performance impact

\- Platform compatibility notes

\- Breaking changes (if any)



Performance: \[specific metrics if applicable]

Platforms: \[tested on which platforms]

```



\*\*Types:\*\* feat, fix, perf, docs, style, refactor, test, build



\*\*Examples:\*\*

```

perf(avx2): optimize xoroshiro128++ batch generation



Implement direct AVX2 intrinsics instead of scalar wrapper.

Reduces function call overhead and improves memory alignment.



Performance: 3.2x speedup in batch mode (was 1.8x)

Platforms: Windows MSVC, Linux GCC

```



---



\## Performance Standards



\### Benchmark Requirements

All performance-related contributions must include:



1\. \*\*Before/After Benchmarks\*\*

&nbsp;  ```

&nbsp;  Algorithm: Xoroshiro128++ AVX2

&nbsp;  Mode: Batch (10,000 values)

&nbsp;  Before: 1.8x speedup vs scalar

&nbsp;  After:  3.2x speedup vs scalar

&nbsp;  Target: 4.0x speedup (theoretical maximum)

&nbsp;  ```



2\. \*\*Cross-Platform Testing\*\*

&nbsp;  - Windows MSVC and MinGW64

&nbsp;  - Linux GCC and Clang

&nbsp;  - macOS (if accessible)



3\. \*\*Regression Testing\*\*

&nbsp;  - No performance regressions in existing implementations

&nbsp;  - Statistical quality maintained (basic randomness tests)



\### Performance Targets

\- \*\*Single Generation:\*\* Match reference implementation speed

\- \*\*AVX2 Batch:\*\* 4x speedup minimum

\- \*\*AVX-512 Batch:\*\* 8x speedup minimum

\- \*\*Memory Usage:\*\* Minimal heap allocation in hot paths

\- \*\*Initialization:\*\* Sub-microsecond RNG setup time



---



\## Testing Requirements



\### Required Tests for New Features

1\. \*\*Correctness:\*\* Algorithm implementation matches specification

2\. \*\*Statistical:\*\* Basic randomness quality (period, distribution)

3\. \*\*Performance:\*\* Meets minimum speedup requirements

4\. \*\*Platform:\*\* Builds and runs on all supported platforms

5\. \*\*Memory:\*\* No leaks, proper cleanup



\### Test Categories

```cpp

// Unit tests

TEST(AlgorithmTest, CorrectImplementation)

TEST(SIMDTest, AVX2Correctness)

TEST(APITest, AllBitWidths)



// Performance tests  

BENCHMARK(SingleGeneration, Xoroshiro128pp)

BENCHMARK(BatchGeneration, AVX2)



// Integration tests

TEST(CrossPlatform, FeatureDetection)

TEST(RuntimeSelection, OptimalImplementation)

```



\### Statistical Quality Requirements

\- \*\*Period:\*\* Full algorithm period achieved

\- \*\*Distribution:\*\* Uniform distribution maintained

\- \*\*Independence:\*\* No correlation between sequential outputs

\- \*\*Basic Tests:\*\* Pass simple statistical tests



---



\## Documentation



\### Code Documentation

\- \*\*Header comments:\*\* Algorithm references and performance notes

\- \*\*Inline comments:\*\* Complex SIMD operations and bit manipulations

\- \*\*API documentation:\*\* Clear parameter descriptions and usage examples



\### Required Documentation for New Features

\- Algorithm description and reference papers

\- Performance characteristics and use cases

\- Platform-specific notes and limitations

\- Example usage code



\### Documentation Tools

\- Doxygen-compatible comments for API documentation

\- Markdown for design documents and guides

\- Performance graphs and charts for optimization results



---



\## Issue Reporting



\### Bug Reports

Include:

\- Platform and compiler details

\- Minimal reproduction case

\- Expected vs actual behavior

\- Performance impact (if applicable)



\### Performance Issues

Include:

\- Benchmark results showing the problem

\- Hardware specifications (CPU model, features)

\- Comparison with theoretical maximum performance

\- Proposed optimization approach (if known)



\### Feature Requests

Include:

\- Use case and motivation

\- Performance requirements

\- Platform requirements

\- Implementation complexity estimate



\### Issue Labels

\- \*\*bug:\*\* Correctness or stability issues

\- \*\*performance:\*\* Optimization opportunities

\- \*\*platform:\*\* Platform-specific problems

\- \*\*simd:\*\* SIMD implementation issues

\- \*\*build:\*\* Build system problems

\- \*\*api:\*\* Interface design discussions



---



\## Development Workflow



\### Pull Request Process

1\. Fork and create feature branch

2\. Implement changes with tests

3\. Run full benchmark suite

4\. Update documentation

5\. Submit PR with performance results

6\. Address review feedback

7\. Merge after approval



\### Review Criteria

\- Correctness of algorithm implementation

\- Performance impact measurement

\- Cross-platform compatibility

\- Code quality and maintainability

\- Documentation completeness



\### Merge Requirements

\- \[ ] All tests pass

\- \[ ] Performance benchmarks provided

\- \[ ] Documentation updated

\- \[ ] Cross-platform compatibility verified

\- \[ ] Code review approval



---



\## Getting Help



\### Communication Channels

\- \*\*Issues:\*\* Technical problems and bug reports

\- \*\*Discussions:\*\* Design decisions and optimization strategies

\- \*\*Email:\*\* \[maintainer contact for sensitive issues]



\### Areas Where Help is Needed

\- \*\*SIMD Optimization:\*\* AVX2/AVX-512 performance tuning

\- \*\*GPU Implementation:\*\* OpenCL/CUDA development

\- \*\*Cross-Platform Testing:\*\* macOS and ARM platform validation

\- \*\*Algorithm Implementation:\*\* New PRNG algorithms

\- \*\*Documentation:\*\* Usage guides and tutorials



---



\## Recognition



Contributors will be recognized in:

\- Project README.md

\- Release notes for significant contributions

\- Performance hall of fame for optimization achievements

\- Algorithm implementation credits



---



\*This contributing guide evolves with the project. Suggestions for improvements are always welcome!\*

