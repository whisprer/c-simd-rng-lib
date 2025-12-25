Further Optimizations to be Considered
AVX2 Specific:

Reduce function call overhead: The Universal RNG uses function pointers, which have more overhead than direct function calls. Consider using template-based dispatch or inline functions where possible.
Optimize batch processing: The batch mode shows impressive gains. You might be able to optimize it further by:

Using larger internal buffers
Reducing memory copies
Implementing more aggressive loop unrolling


Explore compiler optimizations: Try different optimization flags like -O3 -march=native -funroll-loops which can help the compiler generate more efficient code.
Profile the code: Use a profiler to identify the exact bottlenecks. Tools like perf on Linux or Visual Studio's profiler on Windows can show you where the CPU is spending its time.
Consider intrinsics directly: Instead of using a wrapper around the scalar implementation, might get better performance by implementing the core algorithm directly with AVX2 intrinsics.