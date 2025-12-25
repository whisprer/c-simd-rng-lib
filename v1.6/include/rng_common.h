#pragma once
#include <cstdint>
#include <cstddef>
#include <bit>
#include <array>
#include <limits>
#include <new>
#include <type_traits>

#if defined(_MSC_VER)
  #define UA_FORCE_INLINE __forceinline
#else
  #define UA_FORCE_INLINE __attribute__((always_inline)) inline
#endif

#if defined(__cpp_lib_hardware_interference_size)
  using std::hardware_destructive_interference_size;
#else
  // Reasonable fallback (64 on most x86_64/ARM64)
  inline constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

namespace ua {

// ---------- basic utils ----------
UA_FORCE_INLINE constexpr std::uint64_t rotl64(std::uint64_t x, int k) noexcept {
  return (x << k) | (x >> (64 - k));
}

// SplitMix64 for seeding
struct SplitMix64 {
  std::uint64_t state;
  explicit SplitMix64(std::uint64_t s) noexcept : state{s} {}
  UA_FORCE_INLINE std::uint64_t next() noexcept {
    std::uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
  }
};

// wyhash-like 64-bit mix (simple variant)
UA_FORCE_INLINE std::uint64_t mix64(std::uint64_t x) noexcept {
  x ^= x >> 32; x *= 0x9FB21C651E98DF25ULL;
  x ^= x >> 29; x *= 0x9FB21C651E98DF25ULL;
  x ^= x >> 32;
  return x;
}

// Fast [0,1) double from 53-bit mantissa (strict, branchless)
UA_FORCE_INLINE double u64_to_unit_double(std::uint64_t x) noexcept {
  // Take top 53 bits => mantissa, set exponent to 1023 (1.0), subtract 1.0
  const std::uint64_t mant = (x >> 11) | 0x3FF0000000000000ULL;
  double d = std::bit_cast<double>(mant);
  return d - 1.0;
}

// Fast [0,1) float from 24-bit mantissa
UA_FORCE_INLINE float u32_to_unit_float(std::uint32_t x) noexcept {
  const std::uint32_t mant = (x >> 8) | 0x3F800000u;
  float f = std::bit_cast<float>(mant);
  return f - 1.0f;
}

// Buffer policy
struct BufferPolicy {
  std::size_t capacity_u64 = 1024; // default batch
};

} // namespace ua
