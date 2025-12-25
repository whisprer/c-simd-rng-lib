#include <cstddef>
#include <cstdint>
#include <immintrin.h>
#include "rng_common.h"

namespace ua {

#if defined(UA_ENABLE_AVX2)

static constexpr std::uint32_t PHILOX_M0 = 0xD2511F53u;
static constexpr std::uint32_t PHILOX_M1 = 0xCD9E8D57u;
static constexpr std::uint32_t PHILOX_W0 = 0x9E3779B9u;
static constexpr std::uint32_t PHILOX_W1 = 0xBB67AE85u;

static inline __m256i ua_mullo32(__m256i a, __m256i b) { return _mm256_mullo_epi32(a, b); }
static inline __m256i ua_mulhi32(__m256i a, __m256i b) { return _mm256_mulhi_epu32(a, b); }

struct Philox4x32_10_AVX2 {
  std::uint32_t c0, c1, c2, c3;
  std::uint32_t k0, k1;

  explicit Philox4x32_10_AVX2(std::uint64_t seed, std::uint64_t stream) noexcept {
    SplitMix64 sm(seed);
    std::uint64_t k = sm.next();
    k0 = static_cast<std::uint32_t>(k);
    k1 = static_cast<std::uint32_t>(k >> 32);
    std::uint64_t s2 = sm.next();
    c0 = static_cast<std::uint32_t>(stream);
    c1 = static_cast<std::uint32_t>(stream >> 32);
    c2 = static_cast<std::uint32_t>(s2);
    c3 = static_cast<std::uint32_t>(s2 >> 32);
  }

  static inline void add_u128_scalar(std::uint32_t& x0, std::uint32_t& x1,
                                     std::uint32_t& x2, std::uint32_t& x3,
                                     std::uint32_t inc) noexcept {
    std::uint64_t t = static_cast<std::uint64_t>(x0) + inc;
    x0 = static_cast<std::uint32_t>(t);
    std::uint32_t carry = static_cast<std::uint32_t>(t >> 32);
    if (carry) { if (++x1 == 0) { if (++x2 == 0) { ++x3; } } }
  }

  inline void skip_ahead_blocks(std::uint64_t n) noexcept {
    // advance by n blocks (base counter += n)
    // do 32-bit chunks
    std::uint64_t t = static_cast<std::uint64_t>(c0) + (n & 0xFFFFFFFFULL);
    c0 = static_cast<std::uint32_t>(t);
    std::uint32_t carry = static_cast<std::uint32_t>(t >> 32);
    std::uint64_t hi = (n >> 32);
    std::uint64_t t1 = static_cast<std::uint64_t>(c1) + (hi & 0xFFFFFFFFULL) + carry;
    c1 = static_cast<std::uint32_t>(t1); carry = static_cast<std::uint32_t>(t1 >> 32);
    std::uint64_t t2 = static_cast<std::uint64_t>(c2) + (hi >> 32) + carry;
    c2 = static_cast<std::uint32_t>(t2); carry = static_cast<std::uint32_t>(t2 >> 32);
    std::uint64_t t3 = static_cast<std::uint64_t>(c3) + carry;
    c3 = static_cast<std::uint32_t>(t3);
  }

  static inline void build_lane_counters(std::uint32_t base0, std::uint32_t base1,
                                         std::uint32_t base2, std::uint32_t base3,
                                         __m256i& x0, __m256i& x1,
                                         __m256i& x2, __m256i& x3) noexcept {
    alignas(32) std::uint32_t a0[8], a1[8], a2[8], a3[8];
    std::uint32_t t0 = base0, t1 = base1, t2 = base2, t3 = base3;
    for (int lane = 0; lane < 8; ++lane) {
      a0[lane] = t0; a1[lane] = t1; a2[lane] = t2; a3[lane] = t3;
      std::uint64_t s = static_cast<std::uint64_t>(t0) + 1u;
      t0 = static_cast<std::uint32_t>(s);
      std::uint32_t carry = static_cast<std::uint32_t>(s >> 32);
      if (carry) { if (++t1 == 0) { if (++t2 == 0) { ++t3; } } }
    }
    x0 = _mm256_load_si256(reinterpret_cast<const __m256i*>(a0));
    x1 = _mm256_load_si256(reinterpret_cast<const __m256i*>(a1));
    x2 = _mm256_load_si256(reinterpret_cast<const __m256i*>(a2));
    x3 = _mm256_load_si256(reinterpret_cast<const __m256i*>(a3));
  }

  static inline void round_once(__m256i& x0, __m256i& x1, __m256i& x2, __m256i& x3,
                                __m256i k0v, __m256i k1v) noexcept {
    __m256i m0 = _mm256_set1_epi32(static_cast<int>(PHILOX_M0));
    __m256i m1 = _mm256_set1_epi32(static_cast<int>(PHILOX_M1));

    __m256i lo0 = ua_mullo32(m0, x0);
    __m256i hi0 = ua_mulhi32(m0, x0);
    __m256i lo1 = ua_mullo32(m1, x2);
    __m256i hi1 = ua_mulhi32(m1, x2);

    __m256i y0 = _mm256_xor_si256(_mm256_xor_si256(hi1, x1), k0v);
    __m256i y1 = lo1;
    __m256i y2 = _mm256_xor_si256(_mm256_xor_si256(hi0, x3), k1v);
    __m256i y3 = lo0;

    x0 = y0; x1 = y1; x2 = y2; x3 = y3;
  }

  inline void block8_store(std::uint64_t* dst) noexcept {
    __m256i x0, x1, x2, x3;
    build_lane_counters(c0, c1, c2, c3, x0, x1, x2, x3);

    __m256i k0v = _mm256_set1_epi32(static_cast<int>(k0));
    __m256i k1v = _mm256_set1_epi32(static_cast<int>(k1));
    __m256i w0v = _mm256_set1_epi32(static_cast<int>(PHILOX_W0));
    __m256i w1v = _mm256_set1_epi32(static_cast<int>(PHILOX_W1));

    for (int i = 0; i < 9; ++i) { round_once(x0,x1,x2,x3,k0v,k1v); k0v=_mm256_add_epi32(k0v,w0v); k1v=_mm256_add_epi32(k1v,w1v); }
    round_once(x0,x1,x2,x3,k0v,k1v);

    alignas(32) std::uint32_t X0[8], X1[8], X2[8], X3[8];
    _mm256_store_si256(reinterpret_cast<__m256i*>(X0), x0);
    _mm256_store_si256(reinterpret_cast<__m256i*>(X1), x1);
    _mm256_store_si256(reinterpret_cast<__m256i*>(X2), x2);
    _mm256_store_si256(reinterpret_cast<__m256i*>(X3), x3);

    for (int lane = 0; lane < 8; ++lane) {
      dst[2*lane+0] = (static_cast<std::uint64_t>(X1[lane]) << 32) | X0[lane];
      dst[2*lane+1] = (static_cast<std::uint64_t>(X3[lane]) << 32) | X2[lane];
    }
    add_u128_scalar(c0, c1, c2, c3, 8u);
  }

  inline void fill_u64(std::uint64_t* dst, std::size_t n) noexcept {
    while (n >= 16) { block8_store(dst); dst += 16; n -= 16; }
    if (n) { alignas(64) std::uint64_t tmp[16]; block8_store(tmp); for (std::size_t i=0;i<n;++i) dst[i]=tmp[i]; }
  }
};

#endif // UA_ENABLE_AVX2

} // namespace ua
