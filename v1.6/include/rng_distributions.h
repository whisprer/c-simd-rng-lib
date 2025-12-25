#pragma once
#include <cstddef>
#include <cstdint>

namespace ua {

// Scalar Ziggurat for standard normal using 256 layers.
// Tables are computed once at startup (thread-safe).
struct ZigguratNormal {
  static void ensure_init() noexcept;
  static double sample(std::uint64_t u, double u01, double extra_u01) noexcept;

  // Expose read-only table pointers for SIMD gathers
  static const double*      table_x() noexcept; // size 257
  static const double*      table_y() noexcept; // size 256
  static const std::uint32_t* table_k() noexcept; // size 256
};

} // namespace ua
