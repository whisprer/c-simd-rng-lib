#include "simd_normal.h"
#include "rng_distributions.h"
#include "rng_common.h"
#include "simd_exp.h"

#if defined(__aarch64__) || defined(__ARM_NEON)
#include <arm_neon.h>
#define UA_HAVE_NEON 1
#else
#define UA_HAVE_NEON 0
#endif

#include <cmath>

namespace ua {

#if UA_HAVE_NEON

static inline void gather4d(const double* base, const int idx[4], double out[4]) {
  out[0] = base[idx[0]];
  out[1] = base[idx[1]];
  out[2] = base[idx[2]];
  out[3] = base[idx[3]];
}

void simd_normal_neon(const std::uint64_t* u_bits, const std::uint64_t* v_bits,
                      std::size_t n, double mean, double stddev, double* out) {
  const double* x  = ZigguratNormal::table_x();
  const double* y  = ZigguratNormal::table_y();
  const std::uint32_t* k = ZigguratNormal::table_k();

  float64x2_t vmean = vdupq_n_f64(mean);
  float64x2_t vscal = vdupq_n_f64(stddev);

  std::size_t i = 0;
  for (; i + 4 <= n; i += 4) {
    std::uint64_t u64[4] = { u_bits[i+0], u_bits[i+1], u_bits[i+2], u_bits[i+3] };
    std::uint64_t v64[4] = { v_bits[i+0], v_bits[i+1], v_bits[i+2], v_bits[i+3] };
    std::uint32_t u32[4], uu[4], sgn[4], is_tail[4];
    int idxs[4];
    double u01s[4], v01s[4], x_i[4], x_ip1[4], y_i[4], y_ip1[4];

    for (int lane=0; lane<4; ++lane) {
      u32[lane] = static_cast<std::uint32_t>(u64[lane]);
      idxs[lane] = static_cast<int>(u32[lane] & 0xFF);
      is_tail[lane] = (idxs[lane]==0) ? 0xFFFFFFFFu : 0u;
      sgn[lane] = (u32[lane] & 0x100) ? 1u : 0u;
      uu[lane]  = u32[lane] >> 9;
      u01s[lane]= u64_to_unit_double(u64[lane]);
      v01s[lane]= u64_to_unit_double(v64[lane]);
    }

    gather4d(x, idxs, x_i);
    int idxsp1[4] = { idxs[0]+1, idxs[1]+1, idxs[2]+1, idxs[3]+1 };
    gather4d(x, idxsp1, x_ip1);
    gather4d(y, idxs, y_i);
    gather4d(y, idxsp1, y_ip1);

    bool acc_rect[4];
    for (int lane=0; lane<4; ++lane) {
      std::uint32_t kk = k[idxs[lane]];
      acc_rect[lane] = uu[lane] < kk;
    }

    // rect candidates (2 lanes per vector)
    float64x2_t u01_01 = vsetq_lane_f64(u01s[0], vdupq_n_f64(u01s[1]), 0);
    float64x2_t u01_23 = vsetq_lane_f64(u01s[2], vdupq_n_f64(u01s[3]), 0);
    float64x2_t x_i01  = vsetq_lane_f64(x_i[0],  vdupq_n_f64(x_i[1]), 0);
    float64x2_t x_i23  = vsetq_lane_f64(x_i[2],  vdupq_n_f64(x_i[3]), 0);
    float64x2_t rect01 = vfmaq_f64(vmean, vmulq_f64(u01_01, x_i01), vscal);
    float64x2_t rect23 = vfmaq_f64(vmean, vmulq_f64(u01_23, x_i23), vscal);
    double rect_s[4] = {
      vgetq_lane_f64(rect01,0), vgetq_lane_f64(rect01,1),
      vgetq_lane_f64(rect23,0), vgetq_lane_f64(rect23,1)
    };

    // vectorized wedge
    double xw_s[4], ys_s[4], wedge_s[4];
    for (int lane=0; lane<4; ++lane) {
      double dx = (x_i[lane] - x_ip1[lane]);
      double xw = u01s[lane] * dx + x_ip1[lane];
      xw_s[lane] = xw;
      ys_s[lane] = v01s[lane] * (y_ip1[lane] - y_i[lane]) + y_i[lane];
    }
    // compute p(xw) vectorized
    float64x2_t xw01 = vsetq_lane_f64(xw_s[0], vdupq_n_f64(xw_s[1]), 0);
    float64x2_t xw23 = vsetq_lane_f64(xw_s[2], vdupq_n_f64(xw_s[3]), 0);
    float64x2_t p01  = ua_exp_neg_half_x2_neon(xw01);
    float64x2_t p23  = ua_exp_neg_half_x2_neon(xw23);
    double p_w[4] = { vgetq_lane_f64(p01,0), vgetq_lane_f64(p01,1),
                      vgetq_lane_f64(p23,0), vgetq_lane_f64(p23,1) };

    for (int lane=0; lane<4; ++lane) {
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
        double signedx = sgn[lane] ? -xw_s[lane] : xw_s[lane];
        double value_w = mean + stddev * signedx;
        out[i+lane] = (ys_s[lane] < p_w[lane]) ? value_w : ( // accept
          // Box–Muller fallback
          [&](){
            double u1 = u01s[lane] > 0 ? u01s[lane] : 1e-16;
            double u2 = v01s[lane] > 0 ? v01s[lane] : 1e-16;
            double r = std::sqrt(-2.0 * std::log(u1));
            double t = 6.283185307179586476925286766559 * u2;
            double z = r * std::cos(t);
            return mean + stddev * (sgn[lane] ? -z : z);
          }()
        );
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
void simd_normal_neon(const std::uint64_t*, const std::uint64_t*, std::size_t, double, double, double*) {}
#endif

} // namespace ua
