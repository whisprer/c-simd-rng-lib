/README.md
# UA RNG v1.7

A fast, portable **C++20** random number generation library with **runtime SIMD dispatch** and a clean modern API.

- **Scalar baseline:** xoshiro256**
- **AVX2:** 4× lanes
- **AVX-512F:** 8× lanes (optional)
- **Streams:** `u64`, `[0,1)` `double`, `N(0,1)` normal (polar method)
- **Subsequence support:** `jump()` for 2^128 step-ahead
- **Cross-platform:** Linux, macOS, Windows (MSVC / MinGW)

UA RNG is designed to be a **drop-in, high-performance RNG** for Monte Carlo, simulations, procedural content, and anywhere large batches of random numbers are required.

---

## 🚀 Quick Usage

```cpp
#include "ua/ua_rng.h"

int main() {
    ua::Rng rng(1234);  // seed

    std::vector<uint64_t> v(1<<20);
    rng.generate_u64(v.data(), v.size());

    double x = rng.generate_double();
    double n = rng.generate_normal();
}

⚙️ Build
Linux / macOS
cmake -S . -B build -DUA_BUILD_BENCH=ON -DUA_ENABLE_AVX2=ON -DUA_ENABLE_AVX512=ON
cmake --build build -j
./build/ua_rng_bench

Windows (MSVC, Developer PowerShell)
cmake -S . -B build
cmake --build build --config Release
.\build\Release\ua_rng_bench.exe

Runtime Backend Selection

You can force a backend for testing:

UA_FORCE_BACKEND=scalar ./ua_rng_bench
UA_FORCE_BACKEND=avx2   ./ua_rng_bench
UA_FORCE_BACKEND=avx512 ./ua_rng_bench

🎯 Design Notes

Backends live in separate translation units compiled with ISA flags.

ua::Rng façade selects the best backend at runtime via CPUID + OSXSAVE checks.

Doubles use exponent injection (53-bit mantissa) for reproducibility.

Normals use Marsaglia Polar (scalar + AVX2 vectorized rejection).

Optional UA_STREAM_STORES=1 enables non-temporal writes when buffers are aligned.

📊 Version History / Comparison
Feature / Metric			v1.5 (Stable)					v1.6 (Experimental)					v1.7 (Current)
SIMD Support				SSE2, AVX2, AVX-512, NEON			SSE2, AVX2, AVX-512, NEON				Scalar, AVX2, AVX-512F (runtime dispatch)
Algorithms				Xoroshiro128++, WyRand				+ Philox4x32-10	xoshiro256**, normals (Polar)
GPU Support				OpenCL (optional)				OpenCL (partial)					Dropped (CPU SIMD focus, revisit later)
Multi-thread Scaling			Limited	Introduced affinity scaling		Out-of-scope (batch SIMD focus)
Batch Throughput			Excellent					Excellent to Exceptional				4× lanes (AVX2), 8× lanes (AVX-512F)
Single Number Gen.			Competitive, std::mt19937 sometimes	Same	Fast scalar baseline + SIMD batch paths
API Style				C API (handles)	Hybrid experimental modular API	Modern C++ façade (ua::Rng)
Portability				Mature, well-tested				Incomplete OS/arch support				Unified, per-OS builds w/ runtime dispatch
Recommended For	Production workloads	R&D, perf testing				Production workloads needing portability + perf

For full historical details see CHANGELOG.md
.

📦 What You Get

Portability: One static lib works everywhere, SIMD only used when safe.

Speed: 4×–8× throughput on SIMD-capable CPUs.

Normals: Vectorized Polar on AVX2, scalar fallback elsewhere.

Clean API: generate_u64, generate_double, generate_normal, jump, simd_tier().

🔮 Next Perf Pushes

Planned improvements beyond 1.7 include:

Per-backend jump() for AVX paths

Philox4x32-10 AVX2 backend for counter-based parallel streams

Ziggurat normals (AVX2, table-based)

Aligned stream stores with runtime alignment detection

See Development Roadmap
 for details.

📜 License

MIT License – see LICENSE
 file.


---
