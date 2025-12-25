#pragma once
#include <cstdint>

namespace ua {

enum class SimdTier : std::uint8_t {
  Scalar = 0,
  AVX2   = 1,
  AVX512 = 2
};

SimdTier detect_simd();

} // namespace ua
