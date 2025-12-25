#pragma once
#include <cstdint>
#include <cstddef>
#include <cmath>

// Vectorized exp(-0.5 * x^2) for double lanes, header-only.
// Strategy: y = -0.5 * x^2; a = y * log2e; n = round(a); f = a - n in [-0.5,0.5]
// exp(y) = 2^a = 2^n * exp(f * ln2) ; approximate exp(g) on g in [-ln2/2, ln2/2] via 6th-order Estrin.
// We clamp y to [-50, +50] for safety (underflow guard), but typical Ziggurat wedge stays within ~[-6,0].

namespace ua::detail_simd_exp {

static inline double exp_poly_scalar(double g) noexcept {
  // 6th-order polynomial for e^g on g in ~[-0.35, 0.35]
  // P(g) ≈ 1 + g + g^2/2 + g^3/6 + g^4/24 + g^5/120 + g^6/720 (Taylor, good on this range)
  double g2 = g * g;
  double g4 = g2 * g2;
  double p = 1.0
    + g
    + 0.5      * g2
    + (1.0/6.0)* g * g2
    + (1.0/24.0)* g4
    + (1.0/120.0)* g2 * g4
    + (1.0/720.0)* g4 * g2;
  return p;
}

} // namespace ua::detail_simd_exp

// --------------------------- AVX2 ---------------------------
#if defined(UA_ENABLE_AVX2)
  #include <immintrin.h>
namespace ua {

static inline __m256d ua_exp_neg_half_x2_avx2(__m256d x) noexcept {
  // y = -0.5 * x^2
  __m256d y = _mm256_mul_pd(_mm256_set1_pd(-0.5), _mm256_mul_pd(x, x));
  // clamp y to [-50,50] (avoid denorm/overflow)
  __m256d y_clamped = _mm256_max_pd(_mm256_set1_pd(-50.0), _mm256_min_pd(_mm256_set1_pd(50.0), y));

  // a = y * log2(e)
  const __m256d log2e = _mm256_set1_pd(1.4426950408889634073599246810018921);
  __m256d a = _mm256_mul_pd(y_clamped, log2e);

  // n = nearbyint(a)
  __m256d n_d = _mm256_round_pd(a, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
  __m256d f   = _mm256_sub_pd(a, n_d); // f in ~[-0.5,0.5]

  // g = f * ln2
  const __m256d ln2 = _mm256_set1_pd(0.6931471805599453094172321214581766);
  __m256d g = _mm256_mul_pd(f, ln2);

  // Polynomial e^g (Estrin-ish, but we just do direct since deg=6 is small)
  __m256d g2 = _mm256_mul_pd(g, g);
  __m256d g3 = _mm256_mul_pd(g2, g);
  __m256d g4 = _mm256_mul_pd(g2, g2);
  __m256d g5 = _mm256_mul_pd(g4, g);
  __m256d g6 = _mm256_mul_pd(g4, g2);

  __m256d p = _mm256_add_pd(_mm256_set1_pd(1.0), g);
  p = _mm256_add_pd(p, _mm256_mul_pd(_mm256_set1_pd(0.5), g2));
  p = _mm256_add_pd(p, _mm256_mul_pd(_mm256_set1_pd(1.0/6.0), g3));
  p = _mm256_add_pd(p, _mm256_mul_pd(_mm256_set1_pd(1.0/24.0), g4));
  p = _mm256_add_pd(p, _mm256_mul_pd(_mm256_set1_pd(1.0/120.0), g5));
  p = _mm256_add_pd(p, _mm256_mul_pd(_mm256_set1_pd(1.0/720.0), g6));

  // 2^n by constructing exponent bits
  __m256d bias = _mm256_set1_pd(1023.0);
  __m256d n_biased = _mm256_add_pd(n_d, bias);
  // convert to 64-bit ints
  __m256i exp_i = _mm256_cvtpd_epi32(n_biased); // WRONG width; need 64-bit path

  // Proper 64-bit convert: split lanes
  alignas(32) double nd_arr[4];
  _mm256_store_si256(reinterpret_cast<__m256i*>(nd_arr), _mm256_castpd_si256(n_biased)); // store as bits
  // safer: scalar per lane to build exponent bits
  alignas(32) std::uint64_t bits_arr[4];
  for (int i=0;i<4;++i) {
    int64_t nll = (int64_t)llround(nd_arr[i]); // n + 1023  (since n_biased)
    std::uint64_t b = (std::uint64_t)(nll) << 52;
    bits_arr[i] = b;
  }
  __m256i two_n_bits = _mm256_load_si256(reinterpret_cast<const __m256i*>(bits_arr));
  __m256d two_n = _mm256_castsi256_pd(two_n_bits);

  // result = p * 2^n
  return _mm256_mul_pd(p, two_n);
}

} // namespace ua
#endif

// --------------------------- AVX-512 ---------------------------
#if defined(UA_ENABLE_AVX512)
  #include <immintrin.h>
namespace ua {

static inline __m512d ua_exp_neg_half_x2_avx512(__m512d x) noexcept {
  __m512d y = _mm512_mul_pd(_mm512_set1_pd(-0.5), _mm512_mul_pd(x, x));
  __m512d y_clamped = _mm512_max_pd(_mm512_set1_pd(-50.0), _mm512_min_pd(_mm512_set1_pd(50.0), y));

  const __m512d log2e = _mm512_set1_pd(1.44269504088896340736);
  __m512d a = _mm512_mul_pd(y_clamped, log2e);

  __m512d n_d = _mm512_roundscale_pd(a, _MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC);
  __m512d f   = _mm512_sub_pd(a, n_d);

  const __m512d ln2 = _mm512_set1_pd(0.69314718055994530942);
  __m512d g = _mm512_mul_pd(f, ln2);

  __m512d g2 = _mm512_mul_pd(g, g);
  __m512d g3 = _mm512_mul_pd(g2, g);
  __m512d g4 = _mm512_mul_pd(g2, g2);
  __m512d g5 = _mm512_mul_pd(g4, g);
  __m512d g6 = _mm512_mul_pd(g4, g2);

  __m512d p = _mm512_add_pd(_mm512_set1_pd(1.0), g);
  p = _mm512_add_pd(p, _mm512_mul_pd(_mm512_set1_pd(0.5), g2));
  p = _mm512_add_pd(p, _mm512_mul_pd(_mm512_set1_pd(1.0/6.0), g3));
  p = _mm512_add_pd(p, _mm512_mul_pd(_mm512_set1_pd(1.0/24.0), g4));
  p = _mm512_add_pd(p, _mm512_mul_pd(_mm512_set1_pd(1.0/120.0), g5));
  p = _mm512_add_pd(p, _mm512_mul_pd(_mm512_set1_pd(1.0/720.0), g6));

  // 2^n bits: (n + 1023) << 52
  alignas(64) double nd_arr[8];
  _mm512_store_pd(nd_arr, _mm512_add_pd(n_d, _mm512_set1_pd(1023.0)));
  alignas(64) std::uint64_t bits_arr[8];
  for (int i=0;i<8;++i) {
    int64_t nll = (int64_t)llround(nd_arr[i]);
    bits_arr[i] = (std::uint64_t)(nll) << 52;
  }
  __m512i two_n_bits = _mm512_load_si512(reinterpret_cast<const void*>(bits_arr));
  __m512d two_n = _mm512_castsi512_pd(two_n_bits);

  return _mm512_mul_pd(p, two_n);
}

} // namespace ua
#endif

// --------------------------- NEON (AArch64) ---------------------------
#if defined(__aarch64__) || defined(__ARM_NEON)
  #include <arm_neon.h>
namespace ua {

static inline float64x2_t ua_exp_neg_half_x2_neon(float64x2_t x) noexcept {
  float64x2_t y = vmulq_f64(vdupq_n_f64(-0.5), vmulq_f64(x, x));
  // clamp
  y = vmaxq_f64(vdupq_n_f64(-50.0), vminq_f64(vdupq_n_f64(50.0), y));

  // a = y * log2e
  const float64x2_t log2e = vdupq_n_f64(1.44269504088896340736);
  float64x2_t a = vmulq_f64(y, log2e);

  // n = nearbyint(a)
  // AArch64 has vrndnq_f64
  float64x2_t n_d = vrndnq_f64(a);
  float64x2_t f   = vsubq_f64(a, n_d);

  const float64x2_t ln2 = vdupq_n_f64(0.69314718055994530942);
  float64x2_t g = vmulq_f64(f, ln2);

  // poly
  float64x2_t g2 = vmulq_f64(g, g);
  float64x2_t g3 = vmulq_f64(g2, g);
  float64x2_t g4 = vmulq_f64(g2, g2);
  float64x2_t g5 = vmulq_f64(g4, g);
  float64x2_t g6 = vmulq_f64(g4, g2);

  float64x2_t p = vaddq_f64(vdupq_n_f64(1.0), g);
  p = vaddq_f64(p, vmulq_f64(vdupq_n_f64(0.5), g2));
  p = vaddq_f64(p, vmulq_f64(vdupq_n_f64(1.0/6.0), g3));
  p = vaddq_f64(p, vmulq_f64(vdupq_n_f64(1.0/24.0), g4));
  p = vaddq_f64(p, vmulq_f64(vdupq_n_f64(1.0/120.0), g5));
  p = vaddq_f64(p, vmulq_f64(vdupq_n_f64(1.0/720.0), g6));

  // 2^n via exponent bits
  double nd_arr[2]; vst1q_f64(nd_arr, vaddq_f64(n_d, vdupq_n_f64(1023.0)));
  std::uint64_t bits[2];
  bits[0] = (std::uint64_t)((int64_t)llround(nd_arr[0])) << 52;
  bits[1] = (std::uint64_t)((int64_t)llround(nd_arr[1])) << 52;
  float64x2_t two_n = vreinterpretq_f64_u64(vld1q_u64(bits));

  return vmulq_f64(p, two_n);
}

} // namespace ua
#endif
