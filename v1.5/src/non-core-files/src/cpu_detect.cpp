/*  universal-rng / src / cpu_detect.cpp
    -------------------------------------------------------------
    Detect run-time SIMD capabilities on x86-64.

    Public API
    ----------
        bool cpu_supports_sse2();
        bool cpu_supports_avx2();
        bool cpu_supports_avx512();

    These are used by the dispatcher inside universal_rng_api.cpp to
    pick the fastest backend at start-up.  The implementation works
    for both MSVC and GCC/Clang; it contains **no feature-guards**
    that could accidentally disable AVX-2 or AVX-512 in non-MSVC
    builds.
    ------------------------------------------------------------- 
    
    This file now exports BOTH naming styles:
    cpu_supports_*   – new internal API
    detect_*_support – legacy names used in universal_rng_api.cpp
*/

#include <cstdint>

#if defined(_MSC_VER)
    #include <intrin.h>
    static inline void cpuid(uint32_t eax, uint32_t ecx,
                             uint32_t& a, uint32_t& b,
                             uint32_t& c, uint32_t& d)
    {
        int regs[4];
        __cpuidex(regs, int(eax), int(ecx));
        a = uint32_t(regs[0]); b = uint32_t(regs[1]);
        c = uint32_t(regs[2]); d = uint32_t(regs[3]);
    }
#elif defined(__GNUC__)
    #include <cpuid.h>
    static inline void cpuid(uint32_t eax, uint32_t ecx,
                             uint32_t& a, uint32_t& b,
                             uint32_t& c, uint32_t& d)
    {
        __cpuid_count(eax, ecx, a, b, c, d);
    }
#else
    #error "Unsupported compiler"
#endif

/* --- read XCR0 -------------------------------------------------------- */
static inline uint64_t xgetbv0()
{
#if defined(_MSC_VER)
    return _xgetbv(0);
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    uint32_t eax, edx;
    __asm__ volatile(".byte 0x0f, 0x01, 0xd0"    /* xgetbv */
                     : "=a"(eax), "=d"(edx) : "c"(0));
    return (uint64_t(edx) << 32) | eax;
#else
    return 0;
#endif
}

/* --------------------------------------------------------------------- */
/*  Actual capability checks                                             */
/* --------------------------------------------------------------------- */
static bool has_sse2()   { return true; }                    /* mandatory */
static bool has_avx2()
{
    uint32_t eax, ebx, ecx, edx;

    cpuid(1, 0, eax, ebx, ecx, edx);
    if (!(ecx & (1u << 27)) || !(ecx & (1u << 26)))          /* OSXSAVE+XSAVE */
        return false;
    if ((xgetbv0() & 0x6) != 0x6)                            /* XMM|YMM */
        return false;

    cpuid(7, 0, eax, ebx, ecx, edx);
    return (ebx & (1u << 5)) != 0;                           /* AVX2 bit */
}

static bool has_avx512()
{
    if (!has_avx2()) return false;

    if ((xgetbv0() & 0xE0) != 0xE0)                          /* OPMASK|ZMM_hi|Hi16_ZMM */
        return false;

    uint32_t eax, ebx, ecx, edx;
    cpuid(7, 0, eax, ebx, ecx, edx);
    bool f  = (ebx & (1u << 16)) != 0;   /* AVX-512F */
    bool vl = (ebx & (1u << 31)) != 0;   /* AVX-512VL */
    return f && vl;
}

/* --------------------------------------------------------------------- */
/*  Export BOTH APIs so older code links cleanly                         */
/* --------------------------------------------------------------------- */
extern "C" {

bool cpu_supports_sse2()   { return has_sse2();   }
bool cpu_supports_avx2()   { return has_avx2();   }
bool cpu_supports_avx512() { return has_avx512(); }

bool detect_sse2_support()   { return has_sse2();   }
bool detect_avx2_support()   { return has_avx2();   }
bool detect_avx512_support() { return has_avx512(); }

} /* extern "C" */
