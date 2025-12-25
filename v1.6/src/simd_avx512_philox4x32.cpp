#include <cstddef>
#include <cstdint>
#include <immintrin.h>
#include "rng_common.h"

namespace ua {

#if defined(UA_ENABLE_AVX512)

static constexpr std::uint32_t PHILOX_M0 = 0xD2511F53u;
static constexpr std::uint32_t PHILOX_M1 = 0xCD9E8D57u;
static constexpr std::uint32_t PHILOX_W0 = 0x9E3779B9u;
static constexpr std::uint32_t PHILOX_W1 = 0xBB67AE85u;

static inline __m512i ua_mullo32(__m512i a, __m512i b) { return _mm512_mullo_epi32(a, b); }
static inline __m512i ua_mulhi32(__m512i a, __m512i b) { return _mm512_mulhi_epu32(a, b); }

struct Philox4x32_10_AVX512 {
  std::uint32_t c0, c1, c2, c3;
  std::uint32_t k0, k1;

  explicit Philox4x32_10_AVX512(std::uint64_t seed, std::uint64_t stream) noexcept {
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

  inline void skip_ahead_blocks(std::uint64_t n) noexcept {
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

  static inline void build_lane_counters(std::uint32_t b0, std::uint32_t b1,
                                         std::uint32_t b2, std::uint32_t b3,
                                         __m512i& x0, __m512i& x1, __m512i& x2, __m512i& x3) noexcept {
    alignas(64) std::uint32_t A0[16], A1[16], A2[16], A3[16];
    std::uint32_t t0=b0,t1=b1,t2=b2,t3=b3;
    for (int i=0;i<16;++i){
      A0[i]=t0;A1[i]=t1;A2[i]=t2;A3[i]=t3;
      std::uint64_t s=static_cast<std::uint64_t>(t0)+1u;
      t0=static_cast<std::uint32_t>(s);
      std::uint32_t carry=static_cast<std::uint32_t>(s>>32);
      if (carry){ if(++t1==0){ if(++t2==0){ ++t3; } } }
    }
    x0=_mm512_load_si512(reinterpret_cast<const void*>(A0));
    x1=_mm512_load_si512(reinterpret_cast<const void*>(A1));
    x2=_mm512_load_si512(reinterpret_cast<const void*>(A2));
    x3=_mm512_load_si512(reinterpret_cast<const void*>(A3));
  }

  static inline void round_once(__m512i& x0, __m512i& x1, __m512i& x2, __m512i& x3,
                                __m512i k0v, __m512i k1v) noexcept {
    __m512i m0 = _mm512_set1_epi32(static_cast<int>(PHILOX_M0));
    __m512i m1 = _mm512_set1_epi32(static_cast<int>(PHILOX_M1));
    __m512i lo0 = ua_mullo32(m0, x0);
    __m512i hi0 = ua_mulhi32(m0, x0);
    __m512i lo1 = ua_mullo32(m1, x2);
    __m512i hi1 = ua_mulhi32(m1, x2);
    __m512i y0 = _mm512_xor_si512(_mm512_xor_si512(hi1, x1), k0v);
    __m512i y1 = lo1;
    __m512i y2 = _mm512_xor_si512(_mm512_xor_si512(hi0, x3), k1v);
    __m512i y3 = lo0;
    x0=y0;x1=y1;x2=y2;x3=y3;
  }

  inline void block16_store(std::uint64_t* dst) noexcept {
    __m512i x0,x1,x2,x3;
    build_lane_counters(c0,c1,c2,c3,x0,x1,x2,x3);
    __m512i k0v=_mm512_set1_epi32(static_cast<int>(k0));
    __m512i k1v=_mm512_set1_epi32(static_cast<int>(k1));
    __m512i w0v=_mm512_set1_epi32(static_cast<int>(PHILOX_W0));
    __m512i w1v=_mm512_set1_epi32(static_cast<int>(PHILOX_W1));
    for(int i=0;i<9;++i){ round_once(x0,x1,x2,x3,k0v,k1v); k0v=_mm512_add_epi32(k0v,w0v); k1v=_mm512_add_epi32(k1v,w1v); }
    round_once(x0,x1,x2,x3,k0v,k1v);

    alignas(64) std::uint32_t X0[16],X1[16],X2[16],X3[16];
    _mm512_store_si512(reinterpret_cast<void*>(X0),x0);
    _mm512_store_si512(reinterpret_cast<void*>(X1),x1);
    _mm512_store_si512(reinterpret_cast<void*>(X2),x2);
    _mm512_store_si512(reinterpret_cast<void*>(X3),x3);
    for(int lane=0;lane<16;++lane){
      dst[2*lane+0]=(static_cast<std::uint64_t>(X1[lane])<<32)|X0[lane];
      dst[2*lane+1]=(static_cast<std::uint64_t>(X3[lane])<<32)|X2[lane];
    }
    skip_ahead_blocks(16u);
  }

  inline void fill_u64(std::uint64_t* dst, std::size_t n) noexcept {
    while (n>=32){ block16_store(dst); dst+=32; n-=32; }
    if (n){ alignas(64) std::uint64_t tmp[32]; block16_store(tmp); for (std::size_t i=0;i<n;++i) dst[i]=tmp[i]; }
  }
};

#endif // UA_ENABLE_AVX512

} // namespace ua
