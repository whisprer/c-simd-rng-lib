#include <cstddef>
#include <cstdint>
#include <immintrin.h>
#include "rng_common.h"

namespace ua {

#if defined(UA_ENABLE_AVX512)

struct Xoshiro256ssAVX512 {
  __m512i s0, s1, s2, s3; // 8 lanes × 64-bit

  static UA_FORCE_INLINE __m512i rotl64v(__m512i x, int k) noexcept {
    __m512i l = _mm512_slli_epi64(x, k);
    __m512i r = _mm512_srli_epi64(x, 64 - k);
    return _mm512_or_si512(l, r);
  }

  explicit Xoshiro256ssAVX512(std::uint64_t seed, std::uint64_t stream) {
    alignas(64) std::uint64_t a[8], b[8], c[8], d[8];
    for (int i = 0; i < 8; ++i) {
      SplitMix64 sm(seed ^ mix64(stream + std::uint64_t(i)));
      a[i] = sm.next(); b[i] = sm.next(); c[i] = sm.next(); d[i] = sm.next();
      if ((a[i] | b[i] | c[i] | d[i]) == 0) a[i] = 0x9E3779B97F4A7C15ULL;
    }
    s0 = _mm512_load_si512(reinterpret_cast<const void*>(a));
    s1 = _mm512_load_si512(reinterpret_cast<const void*>(b));
    s2 = _mm512_load_si512(reinterpret_cast<const void*>(c));
    s3 = _mm512_load_si512(reinterpret_cast<const void*>(d));
  }

  UA_FORCE_INLINE __m512i next_vec_u64() noexcept {
    // *5 == (x<<2)+x ; *9 == (x<<3)+x
    __m512i x5  = _mm512_add_epi64(_mm512_slli_epi64(s1, 2), s1);
    __m512i rot = rotl64v(x5, 7);
    __m512i res = _mm512_add_epi64(_mm512_slli_epi64(rot, 3), rot);

    __m512i t   = _mm512_slli_epi64(s1, 17);

    s2 = _mm512_xor_si512(s2, s0);
    s3 = _mm512_xor_si512(s3, s1);
    s1 = _mm512_xor_si512(s1, s2);
    s0 = _mm512_xor_si512(s0, s3);

    s2 = _mm512_xor_si512(s2, t);
    s3 = rotl64v(s3, 45);

    return res;
  }

  UA_FORCE_INLINE void fill_u64(std::uint64_t* dst, std::size_t n) noexcept {
    const std::size_t V = 8;
    while (n >= V) {
      __m512i v = next_vec_u64();
      _mm512_storeu_si512(reinterpret_cast<void*>(dst), v);
      dst += V; n -= V;
    }
    if (n) {
      alignas(64) std::uint64_t tmp[8];
      __m512i v = next_vec_u64();
      _mm512_store_si512(reinterpret_cast<void*>(tmp), v);
      for (std::size_t i = 0; i < n; ++i) dst[i] = tmp[i];
    }
  }
};

#endif // UA_ENABLE_AVX512

} // namespace ua
