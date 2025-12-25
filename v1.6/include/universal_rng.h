#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <memory>
#include "rng_common.h"

namespace ua {

enum class Algorithm : std::uint8_t {
  Xoshiro256ss = 0,
  Philox4x32_10 = 1
};

struct alignas(hardware_destructive_interference_size) Init {
  Algorithm   algo   = Algorithm::Xoshiro256ss;
  std::uint64_t seed   = 0xA5A5A5A5A5A5A5A5ULL;
  std::uint64_t stream = 0; // distinct parallel streams
  BufferPolicy buffer{};
};

// C-style thread-local facade
void rng_init(const Init& init);
void rng_set_buffer_capacity(std::size_t capacity_u64);

// Skip ahead by N 128-bit blocks (Philox) or equivalent outputs.
// For Philox, 1 block = 128 bits = 2×u64; for xoshiro we generate/discard.
void rng_skip_ahead_blocks(std::uint64_t nblocks);

std::uint64_t rng_next_u64();
double        rng_next_double();

void rng_generate_u64(std::uint64_t* dst, std::size_t n);
void rng_generate_double(double* dst, std::size_t n);

// Normal distribution (Ziggurat, scalar path) — batched
void rng_generate_normal(double mean, double stddev, double* dst, std::size_t n);

// RAII class
class UniversalRng {
public:
  explicit UniversalRng(const Init& init);
  ~UniversalRng();

  void          skip_ahead_blocks(std::uint64_t nblocks);

  std::uint64_t next_u64();
  double        next_double();
  void          generate_u64(std::uint64_t* dst, std::size_t n);
  void          generate_double(double* dst, std::size_t n);
  void          generate_normal(double mean, double stddev, double* dst, std::size_t n);

private:
  struct Impl;
  std::unique_ptr<Impl> p_;
};

} // namespace ua
