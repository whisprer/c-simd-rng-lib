#include <cstddef>
#include <cstdint>

#if defined(__aarch64__) || defined(__ARM_NEON)
#include <arm_neon.h>
#define UA_HAVE_NEON 1
#else
#define UA_HAVE_NEON 0
#endif

#include "rng_common.h"

namespace ua {

#if UA_HAVE_NEON

static constexpr std::uint32_t PHILOX_M0 = 0xD2511F53u;
static constexpr std::uint32_t PHILOX_M1 = 0xCD9E8D57u;
static constexpr std::uint32_t PHILOX_W0 = 0x9E3779B9u;
static constexpr std::uint32_t PHILOX_W1 = 0xBB67AE85u;

struct Philox4x32_10_NEON {
  std::uint32_t c0, c1, c2, c3;
  std::uint32_t k0, k1;

  explicit Philox4x32_10_NEON(std::uint64_t seed, std::uint64_t stream) noexcept {
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
                                         uint32x4_t& x0, uint32x4_t& x1,
                                         uint32x4_t& x2, uint32x4_t& x3) noexcept {
    std::uint32_t A0[4],A1[4],A2[4],A3[4];
    std::uint32_t t0=b0,t1=b1,t2=b2,t3=b3;
    for(int i=0;i<4;++i){
      A0[i]=t0;A1[i]=t1;A2[i]=t2;A3[i]=t3;
      std::uint64_t s=static_cast<std::uint64_t>(t0)+1u;
      t0=static_cast<std::uint32_t>(s);
      std::uint32_t carry=static_cast<std::uint32_t>(s>>32);
      if (carry){ if(++t1==0){ if(++t2==0){ ++t3; } } }
    }
    x0=vld1q_u32(A0); x1=vld1q_u32(A1); x2=vld1q_u32(A2); x3=vld1q_u32(A3);
  }

  static inline void round_once(uint32x4_t& x0, uint32x4_t& x1, uint32x4_t& x2, uint32x4_t& x3,
                                uint32x4_t k0v, uint32x4_t k1v) noexcept {
    const uint32x4_t m0 = vdupq_n_u32(PHILOX_M0);
    const uint32x4_t m1 = vdupq_n_u32(PHILOX_M1);

    // 32x32 -> 64
    uint64x2_t p0_lo = vmull_u32(vget_low_u32(m0), vget_low_u32(x0));
    uint64x2_t p0_hi = vmull_u32(vget_high_u32(m0), vget_high_u32(x0));
    uint64x2_t p1_lo = vmull_u32(vget_low_u32(m1), vget_low_u32(x2));
    uint64x2_t p1_hi = vmull_u32(vget_high_u32(m1), vget_high_u32(x2));

    uint32x4_t lo0 = vcombine_u32(vmovn_u64(p0_lo), vmovn_u64(p0_hi));
    uint32x4_t hi0 = vcombine_u32(vshrn_n_u64(p0_lo,32), vshrn_n_u64(p0_hi,32));
    uint32x4_t lo1 = vcombine_u32(vmovn_u64(p1_lo), vmovn_u64(p1_hi));
    uint32x4_t hi1 = vcombine_u32(vshrn_n_u64(p1_lo,32), vshrn_n_u64(p1_hi,32));

    uint32x4_t y0 = veorq_u32(veorq_u32(hi1, x1), k0v);
    uint32x4_t y1 = lo1;
    uint32x4_t y2 = veorq_u32(veorq_u32(hi0, x3), k1v);
    uint32x4_t y3 = lo0;

    x0=y0; x1=y1; x2=y2; x3=y3;
  }

  inline void block4_store(std::uint64_t* dst) noexcept {
    uint32x4_t x0,x1,x2,x3;
    build_lane_counters(c0,c1,c2,c3,x0,x1,x2,x3);

    uint32x4_t k0v=vdupq_n_u32(k0);
    uint32x4_t k1v=vdupq_n_u32(k1);
    const uint32x4_t w0v=vdupq_n_u32(PHILOX_W0);
    const uint32x4_t w1v=vdupq_n_u32(PHILOX_W1);

    for (int i=0;i<9;++i){ round_once(x0,x1,x2,x3,k0v,k1v); k0v=vaddq_u32(k0v,w0v); k1v=vaddq_u32(k1v,w1v); }
    round_once(x0,x1,x2,x3,k0v,k1v);

    std::uint32_t X0[4],X1[4],X2[4],X3[4];
    vst1q_u32(X0,x0); vst1q_u32(X1,x1); vst1q_u32(X2,x2); vst1q_u32(X3,x3);
    for (int lane=0; lane<4; ++lane) {
      dst[2*lane+0] = (static_cast<std::uint64_t>(X1[lane])<<32) | X0[lane];
      dst[2*lane+1] = (static_cast<std::uint64_t>(X3[lane])<<32) | X2[lane];
    }
    skip_ahead_blocks(4u);
  }

  inline void fill_u64(std::uint64_t* dst, std::size_t n) noexcept {
    while (n>=8){ block4_store(dst); dst+=8; n-=8; }
    if (n){ std::uint64_t tmp[8]; block4_store(tmp); for (std::size_t i=0;i<n;++i) dst[i]=tmp[i]; }
  }
};

#endif // UA_HAVE_NEON

} // namespace ua
