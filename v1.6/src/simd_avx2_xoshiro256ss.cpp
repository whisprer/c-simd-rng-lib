#include <cstddef>
#include <cstdint>
#include <immintrin.h>
#include "rng_common.h"

namespace ua {

#if defined(UA_ENABLE_AVX2)

struct Xoshiro256ssAVX2 {
  __m256i s0, s1, s2, s3; // 4 lanes × 64-bit: 4 independent streams

  static UA_FORCE_INLINE __m256i rotl64v(__m256i x, int k) noexcept {
    __m256i l = _mm256_slli_epi64(x, k);
    __m256i r = _mm256_srli_epi64(x, 64 - k);
    return _mm256_or_si256(l, r);
  }

  explicit Xoshiro256ssAVX2(std::uint64_t seed, std::uint64_t stream) {
    // Derive 4 lane seeds deterministically from (seed, stream)
    SplitMix64 sm0(seed ^ mix64(stream + 0));
    SplitMix64 sm1(seed ^ mix64(stream + 1));
    SplitMix64 sm2(seed ^ mix64(stream + 2));
    SplitMix64 sm3(seed ^ mix64(stream + 3));

    alignas(32) std::uint64_t a[4], b[4], c[4], d[4];
    for (int i = 0; i < 4; ++i) { a[i] = sm0.next(); b[i] = sm1.next(); c[i] = sm2.next(); d[i] = sm3.next(); }
    // Ensure not all-zero per lane (cheap guard)
    for (int i = 0; i < 4; ++i) {
      if ((a[i] | b[i] | c[i] | d[i]) == 0) a[i] = 0x9E3779B97F4A7C15ULL;
    }
    s0 = _mm256_load_si256(reinterpret_cast<const __m256i*>(a));
    s1 = _mm256_load_si256(reinterpret_cast<const __m256i*>(b));
    s2 = _mm256_load_si256(reinterpret_cast<const __m256i*>(c));
    s3 = _mm256_load_si256(reinterpret_cast<const __m256i*>(d));
  }

  UA_FORCE_INLINE __m256i next_vec_u64() noexcept {
    // result = rotl(s1 * 5, 7) * 9
    __m256i mul5   = _mm256_mul_epu32(s1, _mm256_set1_epi64x(5)); // Note: _mm256_mul_epu32 multiplies 32-bit lanes; but inputs are 64-bit.
    // Correct 64-bit multiplication by 5: (x<<2)+x
    __m256i x4 = _mm256_slli_epi64(s1, 2);
    __m256i x5 = _mm256_add_epi64(x4, s1);
    __m256i rot = rotl64v(x5, 7);
    __m256i res = _mm256_add_epi64(_mm256_slli_epi64(rot, 3), rot); // *9 = (x<<3)+x

    __m256i t   = _mm256_slli_epi64(s1, 17);

    s2 = _mm256_xor_si256(s2, s0);
    s3 = _mm256_xor_si256(s3, s1);
    s1 = _mm256_xor_si256(s1, s2);
    s0 = _mm256_xor_si256(s0, s3);

    s2 = _mm256_xor_si256(s2, t);
    s3 = rotl64v(s3, 45);

    return res;
  }

  UA_FORCE_INLINE void fill_u64(std::uint64_t* dst, std::size_t n) noexcept {
    const std::size_t V = 4; // 4x64 per vector
    while (n >= V) {
      __m256i v = next_vec_u64();
      _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst), v);
      dst += V; n -= V;
    }
    if (n) {
      alignas(32) std::uint64_t tmp[4];
      __m256i v = next_vec_u64();
      _mm256_store_si256(reinterpret_cast<__m256i*>(tmp), v);
      for (std::size_t i = 0; i < n; ++i) dst[i] = tmp[i];
    }
  }
};

#endif // UA_ENABLE_AVX2

} // namespace ua
