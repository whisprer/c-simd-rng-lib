#ifndef OPENCL_RNG_KERNELS_H
#define OPENCL_RNG_KERNELS_H

#include <CL/cl.h>
#include <memory>
#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <iostream>

// Precision mode enum
enum class RNGPrecisionMode {
    SINGLE,
    DOUBLE,
    MIXED
};

// OpenCL RNG Kernel Manager
class OpenCLRNGKernelManager {
public:
    // Static kernel source with modern C++ raw string literal
    static const std::string& getKernelSource() {
        static const std::string kernelSource = R"(
// Utility functions for bit manipulation and rotations
inline ulong rotl64(ulong x, int k) {
    return (x << k) | (x >> (64 - k));
}

// SplitMix64 seeding function
inline ulong splitmix64(ulong z) {
    z += 0x9e3779b97f4a7c15UL;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9UL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebUL;
    return z ^ (z >> 31);
}

// xoroshiro128++ Kernel Implementation
__kernel void xoroshiro128pp_init(
    __global ulong2* states,   // [s0, s1] for each stream
    ulong seed,                // Base seed
    uint num_streams           // Total number of parallel streams
) {
    uint gid = get_global_id(0);
    if (gid >= num_streams) return;

    // Use SplitMix64 to generate initial states
    ulong z = seed + ((ulong)gid * 0x9e3779b97f4a7c15UL);
    ulong s0 = splitmix64(z);
    
    z = s0 + 0x9e3779b97f4a7c15UL;
    ulong s1 = splitmix64(z);

    states[gid] = (ulong2)(s0, s1);
}

__kernel void xoroshiro128pp_generate(
    __global ulong2* states,   // Input/output states
    __global ulong* results,   // Generated random numbers
    uint num_streams           // Total number of parallel streams
) {
    uint gid = get_global_id(0);
    if (gid >= num_streams) return;

    // Load current state
    ulong2 current_state = states[gid];
    ulong s0 = current_state.x;
    ulong s1 = current_state.y;

    // xoroshiro128++ core generation
    ulong result = rotl64(s0 + s1, 17) + s0;
    results[gid] = result;

    // Update state
    s1 ^= s0;
    s0 = rotl64(s0, 49) ^ s1 ^ (s1 << 21);
    s1 = rotl64(s1, 28);

    // Store updated state
    states[gid] = (ulong2)(s0, s1);
}

// Conversion kernels for precision management
__kernel void convert_uint64_to_double(
    __global ulong* input,     // 64-bit unsigned integer input
    __global double* output,   // Double precision output
    uint num_streams           // Total number of streams
) {
    uint gid = get_global_id(0);
    if (gid >= num_streams) return;

    // Convert to double in [0,1) range
    ulong v = input[gid];
    output[gid] = (v >> 11) * (1.0 / (1ULL << 53));
}

__kernel void convert_uint64_to_float(
    __global ulong* input,     // 64-bit unsigned integer input
    __global float* output,    // Single precision output
    uint num_streams           // Total number of streams
) {
    uint gid = get_global_id(0);
    if (gid >= num_streams) return;

    // Convert to float in [0,1) range
    ulong v = input[gid];
    output[gid] = (float)((v >> 11) * (1.0 / (1ULL << 53)));
}
)";
        return kernelSource;
    }

    // Select best GPU device
    static cl_device_id selectBestGPUDevice() {
        cl_platform_id platforms[10];
        cl_uint num_platforms;
        cl_int err;

        // Preferred GPU vendors in order of priority
        const std::vector<std::string> preferred_vendors = {
            "NVIDIA Corporation", 
            "Intel(R) Corporation", 
            "Advanced Micro Devices, Inc."
        };

        // Get available platforms
        err = clGetPlatformIDs(10, platforms, &num_platforms);
        if (err != CL_SUCCESS || num_platforms == 0) {
            std::cerr << "No OpenCL platforms found\n";
            return nullptr;
        }

        // Iterate through preferred vendors
        for (const auto& vendor : preferred_vendors) {
            for (cl_uint p = 0; p < num_platforms; p++) {
                char platform_vendor[128];
                err = clGetPlatformInfo(
                    platforms[p], 
                    CL_PLATFORM_VENDOR, 
                    sizeof(platform_vendor), 
                    platform_vendor, 
                    nullptr
                );

                // Check if current platform matches preferred vendor
                if (std::string(platform_vendor).find(vendor) != std::string::npos) {
                    cl_device_id devices[10];
                    cl_uint num_devices;
                    
                    // Try to get GPU devices
                    err = clGetDeviceIDs(
                        platforms[p], 
                        CL_DEVICE_TYPE_GPU, 
                        10, 
                        devices, 
                        &num_devices
                    );
                    
                    if (err == CL_SUCCESS && num_devices > 0) {
                        // Return first GPU device from this platform
                        return devices[0];
                    }
                }
            }
        }
        
        // No suitable device found
        return nullptr;
    }

    // Validate OpenCL context creation
    static cl_context createOpenCLContext(cl_device_id device) {
        cl_int err;
        cl_context context = clCreateContext(
            nullptr,       // properties
            1,             // number of devices
            &device,       // list of devices
            nullptr,       // pfn_notify callback
            nullptr,       // user data
            &err           // error code
        );

        if (err != CL_SUCCESS) {
            throw std::runtime_error("Failed to create OpenCL context");
        }

        return context;
    }

    // Prevent instantiation
    OpenCLRNGKernelManager() = delete;
    OpenCLRNGKernelManager(const OpenCLRNGKernelManager&) = delete;
    OpenCLRNGKernelManager& operator=(const OpenCLRNGKernelManager&) = delete;
};

#endif // OPENCL_RNG_KERNELS_H
