To improve the implementation:

Review the core generation algorithm for potential optimization
Ensure you're using SIMD instructions effectively
Profile the code to identify any bottlenecks
Compare the implementation details with the standard xoroshiro128+ to understand the performance difference

need to dig deeper into the implementation to understand why there's a performance gap? review the code, get suggestions on specific optimization strategies.


Suggestions for Further Improvement:

Re-implement/finish - logging and/or a verbose mode to show more info.
Potentially profile the code to understand the performance overhead


Recommended Next Steps:

Fix the build system to support AVX-512 detection issue.
Re-introduce runtime selection for specific AVX-512 variants (AVX-512F, AVX-512VL, etc.)
Perform comprehensive benchmarking across different AVX-512 implementations.

Need to further use of std::unique_ptr, std::shared_ptr and similar smart pointer management instead of any remaining raw new/delete or lacking ptrs.
Final checks to ensure consistent memory management across different SIMD implementations
Ensure all SIMD implementation files follow a similar pattern to the scalar implementation


Maybe pursue even higher performance via:

[test AVX-512 support on a capable architecture]
[test openCL support on a capable architecture]
Further batch mode optimizations
Cache-conscious data layout techniques


currently the CPU detection has AVX-512 force disbled - detection issue needs solving so can focus on improving the universal RNG to get better performance.
Looking at your benchmark results, there are a few areas to focus on next:

Speedup in single mode is still poor compared to xoroshiro128+: be nice optimize the AVX2 implementation to get at least parity, if not better performance.
Batch mode speedup is only 1.??x to 2.??x: This suggests there's room for improvement in the batch generation code.

Here are the most promising areas to work on next:

Optimize the AVX2 Implementation

Reduce function call overhead in the critical path
Ensure proper memory alignment for AVX operations


Improve Batch Generation

Enhance the batch mode implementation to fully utilize SIMD parallelism if lacking anywhere.
Consider direct buffer operations instead of individual element access where lacking.
Reduce unnecessary memory copies.


Add Compiler Optimizations

Try different compiler flags to squeeze out more performance
Consider function inlining and loop unrolling where appropriate


Fix Unused Warnings

Clean up any unused code or variables to improve maintainability


I'd recommend starting with the AVX2 implementation to get the base performance up, then moving to batch mode optimizations.



Some possible next steps could be:

Adding more RNG algorithms
Improving the API for specific use cases

Let me know if you have any further ideas this project!