/*  include/avx512_impl.h  */
#pragma once
#include <immintrin.h>
#include <cstdint>
#include <cstddef>
#include <array>
#include <memory>

/* ---------- C ABI – visible from any language ---------- */
#ifdef __cplusplus
extern "C" {
#endif
void*     avx512_new(uint64_t seed);
uint64_t  avx512_next_u64(void* state);
double    avx512_next_double(void* state);
void      avx512_next_batch(void* state, uint64_t* results, size_t count);
void      avx512_free(void* state);
#ifdef __cplusplus
} /* extern "C" */
#endif

/* ---------- C++-only implementation details ------------ */
#ifdef __cplusplus
struct AVX512RNGState {
    static constexpr size_t kBatchSize = 8;

    explicit AVX512RNGState(uint64_t seed);
    static std::unique_ptr<AVX512RNGState> create(uint64_t seed);

    uint64_t next_u64();
    double   next_double();
    void     next_batch(uint64_t* results, size_t count);

    const char* get_implementation_name() const;

private:
    void initialize_state(uint64_t seed);
    void generate_batch();
    static __m512i rotl_avx512(__m512i x, int k);

    alignas(64) std::array<uint64_t, kBatchSize> results_;
    __m512i s0_;
    __m512i s1_;
    size_t  next_idx_;
};
#endif /* __cplusplus */
