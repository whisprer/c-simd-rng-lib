#ifndef SCALAR_IMPL_H
#define SCALAR_IMPL_H

#ifdef __cplusplus
extern "C" {
#endif

void* scalar_new(uint64_t seed);
uint64_t scalar_next_u64(void* state);
double scalar_next_double(void* state);
void scalar_next_batch(void* state, uint64_t* results, size_t count);
void scalar_free(void* state);

#ifdef __cplusplus
}
#endif

#endif // SCALAR_IMPL_H