#include "simd_normal.h"
#include "rng_distributions.h"
#include "rng_common.h"
#include "simd_exp.h"
#include <immintrin.h>

namespace ua {

#if defined(UA_ENABLE_AVX512)

void simd_normal_avx512(const std::uint64_t* u_bits, const std::uint64_t* v_bits,
                        std::size_t n, double mean, double stddev, double* out) {
  const double* x  = ZigguratNormal::table_x();
  const double* y  = ZigguratNormal::table_y();
  const std::uint32_t* k = ZigguratNormal::table_k();

  const __m512d vmean  = _mm512_set1_pd(mean);
  const __m512d vscale = _mm512_set1_pd(stddev);
  const __m512i signbit= _mm512_set1_epi64(0x8000000000000000ULL);

  std::size_t i = 0;
  for (; i + 8 <= n; i += 8) {
    std::uint64_t u64[8], v64[8];
    for (int lane=0; lane<8; ++lane) { u64[lane]=u_bits[i+lane]; v64[lane]=v_bits[i+lane]; }

    std::uint32_t u32[8], uu[8], sgn[8], is_tail[8], kkarr[8];
    int idxs[8]; double u01s[8], v01s[8];
    for (int lane=0; lane<8; ++lane) {
      u32[lane] = static_cast<std::uint32_t>(u64[lane]);
      idxs[lane] = static_cast<int>(u32[lane] & 0xFF);
      is_tail[lane] = (idxs[lane]==0) ? 0xFFFFFFFFu : 0u;
      sgn[lane] = (u32[lane] & 0x100) ? 1u : 0u;
      uu[lane]  = u32[lane] >> 9;
      kkarr[lane]= k[idxs[lane]];
      u01s[lane] = u64_to_unit_double(u64[lane]);
      v01s[lane] = u64_to_unit_double(v64[lane]);
    }

    __m256i vidx   = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(idxs));
    __m512d x_i    = _mm512_i32gather_pd(vidx, x, 8);
    int idxsp1_i[8]= { idxs[0]+1,idxs[1]+1,idxs[2]+1,idxs[3]+1,idxs[4]+1,idxs[5]+1,idxs[6]+1,idxs[7]+1 };
    __m256i vidx1  = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(idxsp1_i));
    __m512d x_ip1  = _mm512_i32gather_pd(vidx1, x, 8);
    __m512d y_i    = _mm512_i32gather_pd(vidx,  y, 8);
    __m512d y_ip1  = _mm512_i32gather_pd(vidx1, y, 8);

    __m512d vu01   = _mm512_loadu_pd(u01s);
    __m512d vabsx  = _mm512_mul_pd(vu01, x_i);

    __m256i vuu    = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(uu));
    __m256i vkk    = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(kkarr));
    __m256i rectcmp= _mm256_cmpgt_epi32(vkk, vuu);
    bool acc_rect[8];
    for (int lane=0; lane<8; ++lane) acc_rect[lane] = (((unsigned)_mm256_movemask_epi8(rectcmp) >> (lane*4)) & 0xF) != 0;

    std::uint64_t sgnmask64[8];
    for (int lane=0; lane<8; ++lane) sgnmask64[lane] = sgn[lane]?0x8000000000000000ULL:0ULL;
    __m512i sgnv = _mm512_loadu_si512(reinterpret_cast<const void*>(sgnmask64));
    __m512d vsigned_rect = _mm512_castsi512_pd(_mm512_xor_si512(_mm512_castpd_si512(vabsx), _mm512_and_si512(sgnv, signbit)));
    __m512d vrect = _mm512_fmadd_pd(vsigned_rect, vscale, vmean);

    // vectorized wedge
    __m512d dx   = _mm512_sub_pd(x_i, x_ip1);
    __m512d x_w  = _mm512_fmadd_pd(vu01, dx, x_ip1);
    __m512d v01  = _mm512_loadu_pd(v01s);
    __m512d dy   = _mm512_sub_pd(y_ip1, y_i);
    __m512d y_s  = _mm512_fmadd_pd(v01, dy, y_i);
    __m512d p_w  = ua_exp_neg_half_x2_avx512(x_w);

    __m512d xw_signed = _mm512_castsi512_pd(_mm512_xor_si512(_mm512_castpd_si512(x_w), _mm512_and_si512(sgnv, signbit)));
    __m512d vwedge = _mm512_fmadd_pd(xw_signed, vscale, vmean);

    alignas(64) double rect_s[8], wedge_s[8], y_s_arr[8], p_w_arr[8];
    _mm512_storeu_pd(rect_s, vrect);
    _mm512_storeu_pd(wedge_s, vwedge);
    _mm512_storeu_pd(y_s_arr, y_s);
    _mm512_storeu_pd(p_w_arr, p_w);

    for (int lane=0; lane<8; ++lane) {
      if (acc_rect[lane]) { out[i+lane] = rect_s[lane]; continue; }
      if (is_tail[lane]) {
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

  for (; i < n; ++i) {
    std::uint64_t u = u_bits[i], v = v_bits[i];
    double u01 = u64_to_unit_double(u);
    double v01 = u64_to_unit_double(v);
    double z = ZigguratNormal::sample(u, u01, v01);
    out[i] = mean + stddev * z;
  }
}

#else
void simd_normal_avx512(const std::uint64_t*, const std::uint64_t*, std::size_t, double, double, double*) {}
#endif

} // namespace ua
