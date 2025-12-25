Key features of the new script:

Compiler auto-detection:

Automatically detects if g++ or MSVC's cl.exe is available
Adjusts flags appropriately for each compiler


Build options:

Debug/Release build modes
Support for building specific targets (wyrand, xoroshiro, comprehensive benchmark)
Ability to force specific SIMD implementations (SSE2, AVX2, AVX-512, OpenCL)
Support for building all SIMD variants at once


AVX-512 Extensions:

Granular control over which AVX-512 extensions to enable


Improved help information:

Detailed usage instructions via the "help" option


OpenCL integration:

Proper linking of OpenCL libraries when needed
Option to disable OpenCL support


Target-specific builds:

Easily build just the WyRand benchmarks, Xoroshiro benchmarks, or comprehensive benchmarks
Default "bench" target builds the universal RNG


compiler.bat                    # Default build with auto-detected SIMD
compiler.bat help               # Show help information
compiler.bat avx2               # Force AVX2 implementation
compiler.bat debug xoroshiro    # Build Xoroshiro benchmarks in debug mode
compiler.bat comprehensive all  # Build all SIMD variants of the comprehensive benchmark