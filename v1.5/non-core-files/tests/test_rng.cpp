#include <cstdio>
#include "universal_rng.h"

int main() {
    universal_rng_t* rng = universal_rng_new(0xDEADBEEF, /*algorithm*/0, /*precision*/0);

    for (int i = 0; i < 10; ++i) {
        uint64_t v = universal_rng_next_u64(rng);
        std::printf("%llu\n", static_cast<unsigned long long>(v));
    }

    universal_rng_free(rng);
    return 0;
}
