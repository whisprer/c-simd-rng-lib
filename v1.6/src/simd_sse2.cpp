#include "sse2_impl.h"
#include <immintrin.h>   // still compile-time-tests SSE2, even if we don’t use intrinsics
#include <cstdlib>
#include <cstring>

/* -------------------------------------------------------------------------
   Simple xoroshiro128+ engine – two 64-bit words → two 64-bit outputs.
   This keeps the code totally portable while satisfying the ABI.
---------------------------------------------------------------------------*/
struct SSE2RNGState {
    uint64_t s0;
    uint64_t s1;
    uint64_t results[2];
    std::size_t next_idx;
};

/* splitmix64 – feeds the seed */
static inline uint64_t splitmix64(uint64_t& x)
{
    uint64_t z = (x += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

static inline uint64_t rotl(uint64_t v, int k)
{
    return (v << k) | (v >> (64 - k));
}

static inline uint64_t next_xoroshiro(SSE2RNGState& st)
{
    const uint64_t result = st.s0 + st.s1;

    uint64_t s1 = st.s1 ^ st.s0;
    st.s0       = rotl(st.s0, 55) ^ s1 ^ (s1 << 14);
    st.s1       = rotl(s1, 36);

    return result;
}

static void generate_batch(SSE2RNGState& st)
{
    st.results[0] = next_xoroshiro(st);
    st.results[1] = next_xoroshiro(st);
    st.next_idx   = 0;
}

/* -------------------------------------------------------------------------
   C-linkage wrappers – exported symbols
---------------------------------------------------------------------------*/
extern "C" {

void* sse2_new(uint64_t seed)
{
    SSE2RNGState* st = static_cast<SSE2RNGState*>(std::malloc(sizeof(SSE2RNGState)));
    if (!st) return nullptr;

    /* initialise two 64-bit state words with splitmix, then pre-fill */
    uint64_t sm = seed;
    st->s0 = splitmix64(sm);
    st->s1 = splitmix64(sm);
    generate_batch(*st);
    return st;
}

uint64_t sse2_next_u64(void* state)
{
    auto* st = static_cast<SSE2RNGState*>(state);
    if (st->next_idx >= 2)
        generate_batch(*st);

    return st->results[st->next_idx++];
}

double sse2_next_double(void* state)
{
    /* convert top 53 bits to double in [0,1) */
    const uint64_t v = sse2_next_u64(state);
    return (v >> 11) * 0x1.0p-53;
}

void sse2_next_batch(void* state, uint64_t* results, std::size_t count)
{
    auto* st = static_cast<SSE2RNGState*>(state);
    for (std::size_t i = 0; i < count; ++i)
        results[i] = sse2_next_u64(st);
}

void sse2_free(void* state)
{
    std::free(state);
}

} // extern "C"
