#ifndef RUNTIME_DETECT_H
#define RUNTIME_DETECT_H

#include "rng_includes.h"
#include <cstdint>
#include <memory>
#include <string>
#include <bitset>
#include <iostream>

// Modern CPU Feature Detection Class
class CPUFeatures {
public:
    // Enum for clear feature representation
    enum class Feature {
        SSE2,
        AVX,
        AVX2,
        NEON,
        AVX512F,
        AVX512DQ,
        AVX512CD,
        AVX512BW,
        AVX512VL,
        AVX512IFMA,
        AVX512VBMI,
        AVX512VBMI2,
        AVX512VNNI,
        AVX512BITALG,
        AVX512VPOPCNTDQ,
        
        // Total number of features
        COUNT
    };

    // Default constructor
    CPUFeatures() {
        detect_features();
    }

    // Check if a specific feature is supported
    [[nodiscard]] bool hasFeature(Feature feature) const {
        return features_.test(static_cast<size_t>(feature));
    }

    // Get feature name as string
    [[nodiscard]] std::string getFeatureName(Feature feature) const {
        static const std::array<std::string, static_cast<size_t>(Feature::COUNT)> feature_names = {
            "SSE2", "AVX", "AVX2", "NEON", 
            "AVX-512F", "AVX-512DQ", "AVX-512CD", "AVX-512BW", 
            "AVX-512VL", "AVX-512IFMA", "AVX-512VBMI", "AVX-512VBMI2", 
            "AVX-512VNNI", "AVX-512BITALG", "AVX-512VPOPCNTDQ"
        };
        return feature_names[static_cast<size_t>(feature)];
    }

    // Print all detected features
    void printFeatures(bool verbose = false) const {
        std::cout << "CPU Feature Detection:\n";
        for (size_t i = 0; i < static_cast<size_t>(Feature::COUNT); ++i) {
            Feature feature = static_cast<Feature>(i);
            bool is_supported = hasFeature(feature);
            
            if (verbose || is_supported) {
                std::cout << "  " << getFeatureName(feature) << ": " 
                          << (is_supported ? "Yes" : "No") << "\n";
            }
        }
    }

    // Static factory method for creating a CPU features instance
    static std::unique_ptr<CPUFeatures> detect(bool silent = false) {
        auto features = std::make_unique<CPUFeatures>();
        if (!silent) {
            features->printFeatures();
        }
        return features;
    }

private:
    // Bitset to store feature support
    std::bitset<static_cast<size_t>(Feature::COUNT)> features_;

    // Platform-specific feature detection
    void detect_features();

    // Platform-specific CPUID detection methods
    #ifdef _WIN32
    void detect_windows_features();
    #elif defined(__linux__) || defined(__APPLE__)
    void detect_unix_features();
    #endif
};

// Alias for backward compatibility
using cpu_features_t = CPUFeatures;

// Inline function for compatibility with existing code
inline void detect_cpu_features(CPUFeatures* features, int silent = 0) {
    if (features) {
        if (!silent) {
            features->printFeatures();
        }
    }
}

#endif // RUNTIME_DETECT_H
