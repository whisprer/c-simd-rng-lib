#include "rng_distributions.h"
#include <atomic>
#include <array>
#include <cmath>
#include <random>
#include <limits>
#include <mutex>

namespace ua {

// Based on Marsaglia & Tsang Ziggurat; compute tables at startup.
static std::once_flag zig_init_flag;
static alignas(64) std::array<double, 257> zig_x; // x[0..256]
static alignas(64) std::array<double, 256> zig_y; // y[0..255]
static alignas(64) std::array<std::uint32_t, 256> zig_k; // acceptance thresholds

static inline double pdf(double x) { return std::exp(-0.5 * x*x); }

void ZigguratNormal::init_once() {
  // Constants chosen per standard references; precompute a 256-layer table.
  const double R = 3.442619855899;
  zig_x[0]   = R;
  zig_y[0]   = pdf(R);
  zig_x[256] = 0.0;

  // Area per rectangle (target total area under the curve split in 256)
  const double A = 0.00492867323399; // empirically tuned constant for 256 layers

  for (int i = 255; i >= 1; --i) {
    zig_x[i] = std::sqrt(-2.0 * std::log(A / zig_x[i+1] + pdf(zig_x[i+1])));
  }
  for (int i = 0; i < 256; ++i) zig_y[i] = pdf(zig_x[i]);
  for (int i = 0; i < 256; ++i) zig_k[i] = static_cast<std::uint32_t>((zig_x[i+1] / zig_x[i]) * 4294967296.0);
}

void ZigguratNormal::ensure_init() noexcept {
  std::call_once(zig_init_flag, &ZigguratNormal::init_once);
}

double ZigguratNormal::sample(std::uint64_t u64, double u01, double extra_u01) noexcept {
  const std::uint32_t u = static_cast<std::uint32_t>(u64);
  const int idx = u & 0xFF;             // layer 0..255
  const std::uint32_t sign = (u & 0x100) ? 1u : 0u;
  const std::uint32_t uu = u >> 9;      // top 23 bits for threshold
  if (uu < zig_k[idx]) {
    double x = u01 * zig_x[idx];
    return sign ? -x : x;
  }

  if (idx == 0) {
    // Tail beyond R
    double x, r;
    do {
      double e = -std::log(u01 + 1e-300) / zig_x[0];
      x = zig_x[0] + e;
      r = std::exp(-0.5 * (x*x - zig_x[0]*zig_x[0]));
    } while (extra_u01 > r);
    return sign ? -x : x;
  } else {
    // Wedge
    double x = u01 * (zig_x[idx] - zig_x[idx+1]) + zig_x[idx+1];
    double y = extra_u01 * (zig_y[idx+1] - zig_y[idx]) + zig_y[idx];
    if (y < pdf(x)) return sign ? -x : x;

    // Rare fallback: Box–Muller
    double u1 = u01 > 0 ? u01 : 1e-16;
    double u2 = extra_u01 > 0 ? extra_u01 : 1e-16;
    double r = std::sqrt(-2.0 * std::log(u1));
    double t = 6.283185307179586476925286766559 * u2; // 2π
    return r * std::cos(t);
  }
}

const double* ZigguratNormal::table_x() noexcept { ensure_init(); return zig_x.data(); }
const double* ZigguratNormal::table_y() noexcept { ensure_init(); return zig_y.data(); }
const std::uint32_t* ZigguratNormal::table_k() noexcept { ensure_init(); return zig_k.data(); }

} // namespace ua
