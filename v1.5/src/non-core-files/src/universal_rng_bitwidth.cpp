#include "universal_rng.h"

// Bit width extension functions - using only the public API
uint16_t universal_rng_next_u16(universal_rng_t* rng) {
    uint64_t val = universal_rng_next_u64(rng);
    return (uint16_t)(val & 0xFFFF);
}

uint32_t universal_rng_next_u32(universal_rng_t* rng) {
    uint64_t val = universal_rng_next_u64(rng);
    return (uint32_t)(val & 0xFFFFFFFF);
}

void universal_rng_next_u128(universal_rng_t* rng, uint64_t* output) {
    if (!rng || !output) return;
    
    for (int i = 0; i < 2; i++) {
        output[i] = universal_rng_next_u64(rng);
    }
}

void universal_rng_next_u256(universal_rng_t* rng, uint64_t* output) {
    if (!rng || !output) return;
    
    for (int i = 0; i < 4; i++) {
        output[i] = universal_rng_next_u64(rng);
    }
}

void universal_rng_next_u512(universal_rng_t* rng, uint64_t* output) {
    if (!rng || !output) return;
    
    for (int i = 0; i < 8; i++) {
        output[i] = universal_rng_next_u64(rng);
    }
}

void universal_rng_next_u1024(universal_rng_t* rng, uint64_t* output) {
    if (!rng || !output) return;
    
    for (int i = 0; i < 16; i++) {
        output[i] = universal_rng_next_u64(rng);
    }
}

// Batch generation implementations
void universal_rng_generate_batch_u16(universal_rng_t* rng, uint16_t* results, size_t count) {
    if (!rng || !results) return;
    
    // For smaller batches, just generate one by one
    if (count <= 16) {
        for (size_t i = 0; i < count; i++) {
            results[i] = universal_rng_next_u16(rng);
        }
        return;
    }
    
    // For larger batches, use 64-bit batch generation and split
    const size_t u64_count = (count + 3) / 4; // Each 64-bit provides 4 16-bit values
    uint64_t* temp = new uint64_t[u64_count];
    
    universal_rng_generate_batch(rng, temp, u64_count);
    
    for (size_t i = 0; i < count; i++) {
        size_t u64_idx = i / 4;
        size_t shift = (i % 4) * 16;
        results[i] = (uint16_t)((temp[u64_idx] >> shift) & 0xFFFF);
    }
    
    delete[] temp;
}

void universal_rng_generate_batch_u32(universal_rng_t* rng, uint32_t* results, size_t count) {
    if (!rng || !results) return;
    
    // Similar approach as for 16-bit, but with 2 values per 64-bit number
    if (count <= 8) {
        for (size_t i = 0; i < count; i++) {
            results[i] = universal_rng_next_u32(rng);
        }
        return;
    }
    
    const size_t u64_count = (count + 1) / 2;
    uint64_t* temp = new uint64_t[u64_count];
    
    universal_rng_generate_batch(rng, temp, u64_count);
    
    for (size_t i = 0; i < count; i++) {
        size_t u64_idx = i / 2;
        size_t shift = (i % 2) * 32;
        results[i] = (uint32_t)((temp[u64_idx] >> shift) & 0xFFFFFFFF);
    }
    
    delete[] temp;
}

void universal_rng_generate_batch_u128(universal_rng_t* rng, uint64_t* results, size_t count) {
    if (!rng || !results) return;
    
    // Each 256-bit number consists of 4 uint64_t values
    const size_t total_u64s = count * 2;
    universal_rng_generate_batch(rng, results, total_u64s);
}

void universal_rng_generate_batch_u256(universal_rng_t* rng, uint64_t* results, size_t count) {
    if (!rng || !results) return;
    
    // Each 256-bit number consists of 4 uint64_t values
    const size_t total_u64s = count * 4;
    universal_rng_generate_batch(rng, results, total_u64s);
}

void universal_rng_generate_batch_u512(universal_rng_t* rng, uint64_t* results, size_t count) {
    if (!rng || !results) return;
    
    // Each 512-bit number consists of 8 uint64_t values
    const size_t total_u64s = count * 8;
    universal_rng_generate_batch(rng, results, total_u64s);
}

void universal_rng_generate_batch_u1024(universal_rng_t* rng, uint64_t* results, size_t count) {
    if (!rng || !results) return;
    
    // Each 1024-bit number consists of 16 uint64_t values
    const size_t total_u64s = count * 16;
    universal_rng_generate_batch(rng, results, total_u64s);
}