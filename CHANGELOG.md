\# Changelog


All notable changes to the Universal RNG Library will be documented in this file.

The format is based on \[Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to \[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

\## \[Unreleased]

\### Added
\- Initial universal RNG architecture with runtime SIMD detection
\- Xoroshiro128++ algorithm implementation (Scalar, SSE2, AVX2, AVX512, NEON)
\- WyRand algorithm implementation (Scalar, SSE2, AVX2, AVX512, NEON)
\- Multi-platform build system (Windows MSVC, MinGW64, Linux GCC)
\- Batch generation capabilities for improved throughput
\- Extended bit-width support (16, 32, 64, 128, 256, 512, 1024 bit)
\- CPU feature detection and automatic optimal implementation selection
\- OpenCL GPU acceleration framework (partial implementation)
\- C API with opaque handle design for cross-language compatibility

\### Changed
\- \[Placeholder for API changes]

\### Fixed
\- CMake SIMD feature detection for MSVC vs GCC/Clang compiler differences
\- AVX2 compile-time vs runtime detection synchronization
\- Cross-platform memory alignment issues


\### Performance
\- AVX2 batch generation showing 1.5-2x speedup over scalar (target: 4x)
\- Runtime implementation selection with zero overhead after initialization


\### Known Issues
\- AVX-512 detection currently disabled due to build system conflicts
\- Shared library (.dll/.so) generation needs refinement
\- Single-value generation performance below reference implementation levels
\- Batch mode not achieving theoretical SIMD parallelism limits

---

\## \[0.1.0] - TBD
\### Added
\- Initial public release
\- Core RNG algorithms: Xoroshiro128++, WyRand
\- Multi-platform SIMD acceleration
\- Universal C API

---

\## Development Notes

\### Version Numbering
\- \*\*Major.Minor.Patch\*\* semantic versioning
\- Major: Breaking API changes
\- Minor: New features, algorithms, or platforms
\- Patch: Bug fixes and performance improvements

\### Release Criteria
\- \[ ] All SIMD implementations compile on target platforms
\- \[ ] Statistical quality tests pass (TestU01, PractRand)
\- \[ ] Performance benchmarks meet minimum thresholds
\- \[ ] Memory leak testing passes
\- \[ ] Cross-platform compatibility verified

\### Future Changelog Sections
As development progresses, consider adding:

\- \*\*Security\*\* - for cryptographically secure algorithm additions
\- \*\*Deprecated\*\* - for API elements being phased out
\- \*\*Removed\*\* - for discontinued features
\- \*\*Dependencies\*\* - for external library changes

