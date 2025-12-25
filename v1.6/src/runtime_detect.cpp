#include "runtime_detect.h"

#if defined(_MSC_VER)
  #include <intrin.h>
#else
  #include <cpuid.h>
#endif

namespace ua {

static void cpuid(int regs[4], int leaf, int subleaf) {
#if defined(_MSC_VER)
  int cpuInfo[4];
  __cpuidex(cpuInfo, leaf, subleaf);
  regs[0] = cpuInfo[0];
  regs[1] = cpuInfo[1];
  regs[2] = cpuInfo[2];
  regs[3] = cpuInfo[3];
#else
  unsigned int a, b, c, d;
  __cpuid_count(leaf, subleaf, a, b, c, d);
  regs[0] = int(a); regs[1] = int(b); regs[2] = int(c); regs[3] = int(d);
#endif
}

SimdTier detect_simd() {
  int r[4]{};
  cpuid(r, 0, 0);
  int max_leaf = r[0];
  if (max_leaf < 7) return SimdTier::Scalar;

  cpuid(r, 7, 0);
  const bool avx2   = (r[1] & (1 << 5)) != 0;      // EBX bit 5
  const bool avx512f= (r[1] & (1 << 16)) != 0;     // EBX bit 16
  const bool avx512dq= (r[1] & (1 << 17)) != 0;    // EBX bit 17
  const bool avx512vl= (r[1] & (1 << 31)) != 0;    // EBX bit 31

#if defined(UA_ENABLE_AVX512)
  if (avx512f && avx512dq && avx512vl) return SimdTier::AVX512;
#endif
#if defined(UA_ENABLE_AVX2)
  if (avx2) return SimdTier::AVX2;
#endif
  return SimdTier::Scalar;
}

} // namespace ua
