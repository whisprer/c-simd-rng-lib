#ifndef CPU_DETECT_H
#define CPU_DETECT_H

#ifdef __cplusplus
extern "C" {
#endif

// CPU feature detection functions
bool detect_avx512_support();
bool detect_avx2_support();
bool detect_sse2_support();

#ifdef __cplusplus
}
#endif

#endif // CPU_DETECT_H