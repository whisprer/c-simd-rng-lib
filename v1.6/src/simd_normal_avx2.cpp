#include "simd_normal.h"
#include "rng_distributions.h"
#include "rng_common.h"
#include "simd_exp.h"
#include <immintrin.h>

namespace ua {

#if defined(UA_ENABLE_AVX2)

static inline __m256d gather_pd(const double* base, const int idx[4]) {
  __m128i vi = _mm_loadu_si128(reinterpret_cast<const __m128i*>(idx));
  return _mm256_i32gather_pd(base, vi, 8);
}

void simd_normal_avx2(const std::uint64_t* u_bits, const std::uint64_t* v_bits,
                      std::size_t n, double mean, double stddev, double* out) {
  const double* x  = ZigguratNormal::table_x();
  const double* y  = ZigguratNormal::table_y();
  const std::uint32_t* k = ZigguratNormal::table_k();

  const __m256d vmean  = _mm256_set1_pd(mean);
  const __m256d vscale = _mm256_set1_pd(stddev);
  const __m256i signbit= _mm256_set1_epi64x(0x8000000000000000ULL);

  std::size_t i = 0;
  for (; i + 4 <= n; i += 4) {
    // lane unpack
    std::uint64_t u64[4] = { u_bits[i+0], u_bits[i+1], u_bits[i+2], u_bits[i+3] };
    std::uint64_t v64[4] = { v_bits[i+0], v_bits[i+1], v_bits[i+2], v_bits[i+3] };

    std::uint32_t u32[4], uu[4], sgn[4], is_tail[4], kkarr[4];
    int idxs[4];
    double u01s[4], v01s[4];

    for (int lane=0; lane<4; ++lane) {
      u32[lane] = static_cast<std::uint32_t>(u64[lane]);
      idxs[lane] = static_cast<int>(u32[lane] & 0xFF);
      is_tail[lane] = (idxs[lane]==0) ? 0xFFFFFFFFu : 0u;
      sgn[lane] = (u32[lane] & 0x100) ? 1u : 0u;
      uu[lane]  = u32[lane] >> 9;
      kkarr[lane] = k[idxs[lane]];
      u01s[lane] = u64_to_unit_double(u64[lane]);
      v01s[lane] = u64_to_unit_double(v64[lane]);
    }

    // gathers
    __m256d x_i   = gather_pd(x, idxs);
    int idxsp1[4] = { idxs[0]+1, idxs[1]+1, idxs[2]+1, idxs[3]+1 };
    __m256d x_ip1 = gather_pd(x, idxsp1);
    __m256d y_i   = gather_pd(y, idxs);
    __m256d y_ip1 = gather_pd(y, idxsp1);

    // rectangle candidate and accept mask
    __m256d vu01  = _mm256_loadu_pd(u01s);
    __m256d vabsx = _mm256_mul_pd(vu01, x_i);

    __m128i uu128 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(uu));
    __m128i kk128 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(kkarr));
    __m128i rect_cmp = _mm_cmplt_epi32(uu128, kk128);
    bool acc_rect[4] = {
      _mm_extract_epi32(rect_cmp,0)!=0,
      _mm_extract_epi32(rect_cmp,1)!=0,
      _mm_extract_epi32(rect_cmp,2)!=0,
      _mm_extract_epi32(rect_cmp,3)!=0
    };

    std::uint64_t sgnmask64[4];
    for (int lane=0; lane<4; ++lane) sgnmask64[lane] = sgn[lane]?0x8000000000000000ULL:0ULL;
    __m256i sgnv = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(sgnmask64));
    __m256d vsigned_rect = _mm256_castsi256_pd(_mm256_xor_si256(_mm256_castpd_si256(vabsx), _mm256_and_si256(sgnv, signbit)));
    __m256d vrect = _mm256_fmadd_pd(vsigned_rect, vscale, vmean);

    // vectorized wedge:
    // x_w = u01 * (x[i]-x[i+1]) + x[i+1]
    __m256d dx   = _mm256_sub_pd(x_i, x_ip1);
    __m256d x_w  = _mm256_fmadd_pd(vu01, dx, x_ip1);
    // y_samp = v01 * (y[i+1]-y[i]) + y[i]
    __m256d v01  = _mm256_loadu_pd(v01s);
    __m256d dy   = _mm256_sub_pd(y_ip1, y_i);
    __m256d y_s  = _mm256_fmadd_pd(v01, dy, y_i);
    // p(x_w) = exp(-0.5 * x_w^2), fully vectorized
    __m256d p_w  = ua_exp_neg_half_x2_avx2(x_w);

    // signed x_w
    __m256d xw_signed = _mm256_castsi256_pd(_mm256_xor_si256(_mm256_castpd_si256(x_w), _mm256_and_si256(sgnv, signbit)));
    __m256d vwedge = _mm256_fmadd_pd(xw_signed, vscale, vmean);

    // choose per-lane: rect if acc_rect; else tail? fallback to scalar tail (very rare); else wedge accept if y_s < p_w; else Box–Muller scalar
    alignas(32) double rect_s[4], wedge_s[4], y_s_arr[4], p_w_arr[4];
    _mm256_storeu_pd(rect_s, vrect);
    _mm256_storeu_pd(wedge_s, vwedge);
    _mm256_storeu_pd(y_s_arr, y_s);
    _mm256_storeu_pd(p_w_arr, p_w);

    for (int lane=0; lane<4; ++lane) {
      if (acc_rect[lane]) { out[i+lane] = rect_s[lane]; continue; }
      if (is_tail[lane]) {
        // still scalar for tail (super rare)
        double R = x[0];
        double e = -std::log(u01s[lane] + 1e-300) / R;
        double xv = R + e;
        double r  = std::exp(-0.5 * (xv*xv - R*R));
        while (v01s[lane] > r) {
          std::uint64_t z = u64[lane] = u64[lane] * 6364136223846793005ULL + 1ULL;
          u01s[lane] = u64_to_unit_double(z);
          v01s[lane] = u64_to_unit_double(z ^ (z>>33));
          e = -std::log(u01s[lane] + 1e-300) / R;
          xv = R + e;
          r  = std::exp(-0.5 * (xv*xv - R*R));
        }
        double signedx = sgn[lane] ? -xv : xv;
        out[i+lane] = mean + stddev * signedx;
      } else {
        if (y_s_arr[lane] < p_w_arr[lane]) {
          out[i+lane] = wedge_s[lane];
        } else {
          // Box–Muller fallback
          double u1 = u01s[lane] > 0 ? u01s[lane] : 1e-16;
          double u2 = v01s[lane] > 0 ? v01s[lane] : 1e-16;
          double r = std::sqrt(-2.0 * std::log(u1));
          double t = 6.283185307179586476925286766559 * u2;
          double z = r * std::cos(t);
          out[i+lane] = mean + stddev * (sgn[lane] ? -z : z);
        }
      }
    }
  }

  // remainder
  for (; i < n; ++i) {
    std::uint64_t u = u_bits[i], v = v_bits[i];
    double u01 = u64_to_unit_double(u);
    double v01 = u64_to_unit_double(v);
    double z = ZigguratNormal::sample(u, u01, v01);
    out[i] = mean + stddev * z;
  }
}

#else
void simd_normal_avx2(const std::uint64_t*, const std::uint64_t*, std::size_t, double, double, double*) {}
#endif

} // namespace ua
