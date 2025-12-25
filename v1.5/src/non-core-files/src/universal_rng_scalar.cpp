#include "rng_includes.h"
#include "universal_rng.h"
#include <memory>
#include <cstdint>
#include <stdexcept>

// Modern C++ Scalar RNG Implementation
class ScalarRNGState {
public:
    // Constructors and factory method
    explicit ScalarRNGState(uint64_t seed) {
        seed_state(seed);
    }

    // Static factory method
    static std::unique_ptr<ScalarRNGState> create(uint64_t seed) {
        return std::make_unique<ScalarRNGState>(seed);
    }

    // Next 64-bit random number generation
    uint64_t next_u64() {
        const uint64_t s0 = state[0];
        uint64_t s1 = state[1];
        
        const uint64_t result = rotl(s0 + s1, 17) + s0;
        
        s1 ^= s0;
        state[0] = rotl(s0, 49) ^ s1 ^ (s1 << 21);
        state[1] = rotl(s1, 28);
        
        return result;
    }

    // Next double in [0,1) range
    double next_double() {
        uint64_t v = next_u64();
        return (v >> 11) * (1.0 / (1ULL << 53));
    }

    // Batch generation
    void next_batch(uint64_t* results, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            results[i] = next_u64();
        }
    }

private:
    // Seeding using SplitMix64
    void seed_state(uint64_t seed) {
        uint64_t z = (seed + 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        state[0] = z ^ (z >> 31);

        z = (state[0] + 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        state[1] = z ^ (z >> 31);
    }

    // Bit rotation helper (inline for performance)
    static inline uint64_t rotl(uint64_t x, int k) {
        return (x << k) | (x >> (64 - k));
    }

    // State storage
    uint64_t state[2];
};

// C-compatible function implementations
extern "C" {
    void* scalar_new(uint64_t seed) {
        return ScalarRNGState::create(seed).release();
    }

    uint64_t scalar_next_u64(void* state) {
        return static_cast<ScalarRNGState*>(state)->next_u64();
    }

    double scalar_next_double(void* state) {
        return static_cast<ScalarRNGState*>(state)->next_double();
    }

    void scalar_next_batch(void* state, uint64_t* results, size_t count) {
        static_cast<ScalarRNGState*>(state)->next_batch(results, count);
    }

    void scalar_free(void* state) {
        delete static_cast<ScalarRNGState*>(state);
    }
}