ok fren, slide sidetrack question -  right now i'm coding all this on a 2017 ms surface pro - i.e. it doesn't have SIMD-optimization capabilities as far as AVX etc. goes. is there anything i can take advantage of such as the gfx/cpu special combo setup or something?EditThat's a great question! Your 2017 Surface Pro actually does have some SIMD capabilities we can leverage, even if it doesn't support the latest AVX extensions.
Surface Pro 2017 models typically came with Intel Core i5-7300U or i7-7660U processors from the Kaby Lake generation. These processors support:

SSE (Streaming SIMD Extensions) - All versions up to SSE4.2
AVX (Advanced Vector Extensions) - Basic AVX 256-bit instructions
Intel HD Graphics 620/Iris Plus Graphics 640 - These integrated GPUs have their own parallel processing capabilities

Here's what you can take advantage of:
1. SSE Instructions
You can use SSE instructions which provide 128-bit SIMD operations. These are still very powerful and can give you significant speedups compared to scalar code.
cppCopy// Example of a SSE implementation for xoroshiro128++
#ifdef __SSE2__
typedef struct {
    __m128i state0;  // 2 streams, first state component
    __m128i state1;  // 2 streams, second state component
} xoroshiro128pp_sse2;

// Helper for SSE rotl
static inline __m128i rotl_sse2(__m128i x, int k) {
    return _mm_or_si128(_mm_slli_epi64(x, k), _mm_srli_epi64(x, 64 - k));
}

// Generate 2 random 64-bit values in parallel
static void xoroshiro128pp_sse2_next(xoroshiro128pp_sse2 *state, uint64_t results[2]) {
    // Similar implementation to your NEON version, but with SSE intrinsics
    __m128i sum = _mm_add_epi64(state->state0, state->state1);
    __m128i rotated_sum = rotl_sse2(sum, 17);
    __m128i result = _mm_add_epi64(rotated_sum, state->state0);
    
    // Store results
    _mm_storeu_si128((__m128i*)results, result);
    
    // Update state
    __m128i s1 = _mm_xor_si128(state->state0, state->state1);
    
    state->state0 = _mm_xor_si128(
                      rotl_sse2(state->state0, 49),
                      _mm_xor_si128(
                          s1,
                          _mm_slli_epi64(s1, 21)
                      )
                    );
                    
    state->state1 = rotl_sse2(s1, 28);
}
#endif
2. Basic AVX
Your processor supports basic AVX instructions (though not AVX2 or AVX-512). You could enable this in your code with -mavx flag.
3. OpenCL for GPU Acceleration
For truly parallelized random number generation, you could use OpenCL to offload work to the integrated GPU:
cppCopy// Conceptual example, not full implementation
const char* opencl_kernel = R"(
    kernel void xoroshiro128pp_kernel(global ulong* state0, global ulong* state1, 
                                     global ulong* results, const int count) {
        int id = get_global_id(0);
        if (id < count) {
            // xoroshiro128++ implementation for each work item
            ulong s0 = state0[id];
            ulong s1 = state1[id];
            
            // Calculate result with ++ scrambler
            results[id] = rotl(s0 + s1, 17) + s0;
            
            // Update state
            s1 ^= s0;
            state0[id] = rotl(s0, 49) ^ s1 ^ (s1 << 21);
            state1[id] = rotl(s1, 28);
        }
    }
)";
4. Modified Compilation Script
To take advantage of the available SIMD extensions on your Surface Pro, modify your compilation script:
batchCopy:: For Surface Pro 2017 (Kaby Lake processors)
SET CXXFLAGS=-std=c++11 -O3 -Wall -msse4.2 -mavx
SET DEFS=-D__SSE2__ -D__SSE3__ -D__SSSE3__ -D__SSE4_1__ -D__SSE4_2__ -D__AVX__
Practical Advice

Start with SSE4.2: This will give you good performance with 128-bit SIMD operations, which is still 2x the scalar performance.
Use conditional compilation: Keep your code adaptable to different architectures by using preprocessor conditionals:
cppCopy#if defined(__AVX512F__)
    // AVX-512 code
#elif defined(__AVX2__)
    // AVX2 code
#elif defined(__AVX__)
    // Basic AVX code
#elif defined(__SSE4_2__)
    // SSE4.2 code
#else
    // Scalar fallback
#endif

Runtime detection: For a distributable program, implement runtime CPU feature detection rather than compile-time only.

Your Surface Pro has enough SIMD capabilities to see meaningful speedups over scalar code - you'd just be using 128-bit or 256-bit operations instead of the wider 512-bit ones in newer processors.
Would you like me to help you implement a specific SSE-based version that would work well on your Surface Pro?