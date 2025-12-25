#include <cstdint>
#include "rng_common.h"

namespace ua {

// Scalar xoshiro256** core (public domain reference adapted)
struct Xoshiro256ssScalar {
  std::uint64_t s0, s1, s2, s3;

  explicit Xoshiro256ssScalar(std::uint64_t seed, std::uint64_t stream) {
    SplitMix64 sm(seed ^ mix64(stream));
    s0 = sm.next(); s1 = sm.next(); s2 = sm.next(); s3 = sm.next();
    // avoid all-zero
    if ((s0 | s1 | s2 | s3) == 0) s0 = 0x9E3779B97F4A7C15ULL;
  }

  UA_FORCE_INLINE std::uint64_t next_u64() noexcept {
    const std::uint64_t result = rotl64(s1 * 5ULL, 7) * 9ULL;

    const std::uint64_t t = s1 << 17;

    s2 ^= s0;
    s3 ^= s1;
    s1 ^= s2;
    s0 ^= s3;

    s2 ^= t;
    s3 = rotl64(s3, 45);
    return result;
  }

  UA_FORCE_INLINE void fill_u64(std::uint64_t* dst, std::size_t n) noexcept {
    for (std::size_t i = 0; i < n; ++i) dst[i] = next_u64();
  }
};

} // namespace ua
