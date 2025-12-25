
Universal SIMD-optimized xoroshiro128++ Compilation Script

This script creates a binary that automatically selects the best SIMD implementation at runtime for your hardware

The script will automatically:

- Detect your available compiler (g++, clang++, or MSVC)
- Check for available SIMD instruction sets (AVX-512, AVX2, AVX, SSE2)
- Check for OpenCL support
- Build the optimized code with the best available implementation
- Auto-run a benchmark comparing the original algorithm with the SIMD-optimized one


Usage:
  compile           - Build universal binary with runtime detection
  compile avx2      - Force AVX2 implementation
  compile avx512    - Force AVX-512 implementation
  compile sse2      - Force SSE2 implementation
  compile opencl    - Force OpenCL implementation (needs SDK)
  compile all       - Build all available variants
  compile no-opencl - Build without checking for OpenCL

  compile avx512f avx512dq - use multiples to get basics plus double and e.g. quad wrd

  compile avx512f avx512dq avx512bw avx512vl - likewse, for all the trimmings - keep 					       adding

Full List of AVX-512 Instruction Set Extensions:

Force avx512f	    - Foundation
Add avx512cd	    - Conflict Detection
Add avx512bw	    - Byte and Word
Add avx512dq	    - Doubleword and Quadword
Add avx512vl	    - Vector Length
Add avx512ifma	    - Integer Fused Multiply-Add
Add avx512vbmi	    - Vector Byte Manipulation Instructions
Add avx512vbmi	    - Vector Byte Manipulation Instructions 2
Add avx512vnni	    - Vector Neural Network Instructions
Add avx512bitalg    - Bit Algorithms
dd avx512vpopcntdq  - Population Count



all these files need to be in same directory for current config to work:


Batch Files

compiler.bat (4 KB)


C/C++ Source Files

advanced_benchmarking.h.c (6 KB)
exmaple_usage.cpp (1 KB)
logging_temp.cpp (1 KB)
main_bench.cpp (1 KB)
multi_algorithm_comparison.c (3 KB)
opencl_isi.cpp (9 KB)
runtime_detect.cpp (4 KB)
simd_avx2.cpp (6 KB)
simd_avx512.cpp (20 KB)
simd_avx512-bak.cpp (8 KB)
simd_sse2.cpp (6 KB)
universal_bench.cpp (5 KB)
universal_rng.cpp (4 KB)
universal_rng_scalar.cpp (2 KB)
wyrand_rng_bench.cpp (1 KB)
wyrand_rng_bench_optimized.cpp (1 KB)
xoroshiro128pp_rng_bench.cpp (1 KB)
xoroshiro128pp_rng_bench_optimized.cpp (1 KB)


C/C++ Header Files

gpu_optimization_detection.h (6 KB)
opencl_rng_integration.h (4 KB)
opencl_rng_kernels.h (23 KB)
rng_benchmark.h (4 KB)
runtime_detect.h (2 KB)
universal_rng.h (3 KB)
verbose_mode.h (2 KB)
wyrand_rng_bench.h (6 KB)
wyrand_rng_bench_optimized.h (5 KB)
wyrand_simd_main.h (11 KB)
wyrand_simd_main_optimized.h (27 KB)
xoroshiro128pp_rng_bench.h (6 KB)
xoroshiro128pp_rng_bench_optimized.h (5 KB)
xoroshiro128pp_simd_main.h (16 KB)
xoroshiro128pp_simd_main_optimized.h (26 KB)


[Executable Files

universal_bench.exe (260 KB) ]



in win10/11 terminal, a nor or admmin [unless previously setup]:

`.\compile.bat`

[unless specifically needing a certain implementation forced or bypassing openCL]
[[see instructions .md for full listings of compile commands and extensions]]


