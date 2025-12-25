#include "rng_includes.h"
#include "runtime_detect.h"

#ifdef _WIN32
#include <intrin.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <cpuid.h>
#endif

// Implementation of feature detection
void CPUFeatures::detect_features() {
    #ifdef _WIN32
        detect_windows_features();
    #elif defined(__linux__) || defined(__APPLE__)
        detect_unix_features();
    #endif

    // ARM NEON detection
    #ifdef __ARM_NEON
        features_.set(static_cast<size_t>(Feature::NEON));
    #endif
}

#ifdef _WIN32
void CPUFeatures::detect_windows_features() {
    int cpu_info[4] = {0};
    
    // Check max supported function
    __cpuid(cpu_info, 0);
    int max_func = cpu_info[0];
    
    // Get features
    if (max_func >= 1) {
        __cpuid(cpu_info, 1);
        
        // SSE2
        if (cpu_info[3] & (1 << 26)) 
            features_.set(static_cast<size_t>(Feature::SSE2));
        
        // AVX
        if (cpu_info[2] & (1 << 28)) 
            features_.set(static_cast<size_t>(Feature::AVX));
        
        // Check for OSXSAVE bit - required for AVX
        bool osxsave = (cpu_info[2] & (1 << 27)) != 0;
        
        // Only check AVX2 and AVX-512 if AVX is supported and OSXSAVE is set
        if (hasFeature(Feature::AVX) && osxsave && max_func >= 7) {
            int extended_info[4] = {0};
            __cpuidex(extended_info, 7, 0);
            
            // AVX2
            if (extended_info[1] & (1 << 5)) 
                features_.set(static_cast<size_t>(Feature::AVX2));
            
            // AVX-512 Foundation
            if (extended_info[1] & (1 << 16)) {
                features_.set(static_cast<size_t>(Feature::AVX512F));
                
                // Additional AVX-512 feature checks
                if (extended_info[1] & (1 << 17)) 
                    features_.set(static_cast<size_t>(Feature::AVX512DQ));
                if (extended_info[1] & (1 << 28)) 
                    features_.set(static_cast<size_t>(Feature::AVX512CD));
                if (extended_info[1] & (1 << 30)) 
                    features_.set(static_cast<size_t>(Feature::AVX512BW));
                if (extended_info[1] & (1 << 31)) 
                    features_.set(static_cast<size_t>(Feature::AVX512VL));
                
                // More AVX-512 features
                if (extended_info[1] & (1 << 21)) 
                    features_.set(static_cast<size_t>(Feature::AVX512IFMA));
                if (extended_info[2] & (1 << 1)) 
                    features_.set(static_cast<size_t>(Feature::AVX512VBMI));
                if (extended_info[2] & (1 << 6)) 
                    features_.set(static_cast<size_t>(Feature::AVX512VBMI2));
                if (extended_info[2] & (1 << 11)) 
                    features_.set(static_cast<size_t>(Feature::AVX512VNNI));
                if (extended_info[2] & (1 << 12)) 
                    features_.set(static_cast<size_t>(Feature::AVX512BITALG));
                if (extended_info[2] & (1 << 14)) 
                    features_.set(static_cast<size_t>(Feature::AVX512VPOPCNTDQ));
            }
        }
    }
}
#elif defined(__linux__) || defined(__APPLE__)
void CPUFeatures::detect_unix_features() {
    // Placeholder for Unix-based CPUID detection
    // This would need to be implemented similarly to the Windows version
    // using __get_cpuid() from <cpuid.h>
    // Details omitted for brevity
}
#endif
