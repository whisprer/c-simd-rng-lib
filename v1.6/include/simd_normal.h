#pragma once
#include <cstddef>
#include <cstdint>

namespace ua {

void simd_normal_avx2(const std::uint64_t* u_bits, const std::uint64_t* v_bits,
                      std::size_t n, double mean, double stddev, double* out);

void simd_normal_avx512(const std::uint64_t* u_bits, const std::uint64_t* v_bits,
                        std::size_t n, double mean, double stddev, double* out);

void simd_normal_neon(const std::uint64_t* u_bits, const std::uint64_t* v_bits,
                      std::size_t n, double mean, double stddev, double* out);

} // namespace ua
