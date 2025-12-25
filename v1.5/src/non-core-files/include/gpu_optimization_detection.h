#ifndef GPU_OPTIMIZATION_DETECTION_H
#define GPU_OPTIMIZATION_DETECTION_H

#include <CL/cl.h>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <iomanip>

// Modern C++ GPU Device Capabilities Class
class GPUDeviceCapabilities {
public:
    // Constructors and factory methods
    GPUDeviceCapabilities() = default;

    // Static method to detect GPU capabilities
    static std::unique_ptr<GPUDeviceCapabilities> detect() {
        auto capabilities = std::make_unique<GPUDeviceCapabilities>();
        if (!capabilities->initialize()) {
            return nullptr;
        }
        return capabilities;
    }

    // Print device information
    void printDetails(std::ostream& os = std::cout) const {
        os << "GPU Device Information:\n"
           << "---------------------\n"
           << "Device:            " << device_name_ << "\n"
           << "Vendor:            " << vendor_ << "\n"
           << "\nComputational Capabilities:\n"
           << "  Global Memory:   " << formatMemorySize(global_mem_size_) << "\n"
           << "  Max Allocation:  " << formatMemorySize(max_mem_alloc_size_) << "\n"
           << "  Compute Units:   " << max_compute_units_ << "\n"
           << "  Max Work Group:  " << max_work_group_size_ << "\n"
           << "\nFeature Support:\n"
           << "  Double Precision: " << (supports_double_precision_ ? "Yes" : "No") << "\n"
           << "  Local Memory:     " << (supports_local_memory_ ? "Yes" : "No") << "\n"
           << "  Unified Memory:   " << (supports_unified_memory_ ? "Yes" : "No") << "\n"
           << "\nPerformance:\n"
           << "  Clock Frequency: " << max_clock_frequency_ << " MHz\n"
           << "\nMulti-GPU:\n"
           << "  Multi-GPU Support: " << (supports_multi_gpu_ ? "Yes" : "No") << "\n"
           << "  Available Devices: " << num_available_devices_ << "\n";
    }

    // Recommendation generator
    std::vector<std::string> getOptimizationRecommendations() const {
        std::vector<std::string> recommendations;

        if (max_compute_units_ > 8) {
            recommendations.push_back(
                "High parallelism detected. Consider increasing batch size for better GPU utilization."
            );
        }

        if (global_mem_size_ > (16ULL * 1024 * 1024 * 1024)) {
            recommendations.push_back(
                "Large GPU memory. Explore more memory-intensive random number generation techniques."
            );
        }

        if (!supports_double_precision_) {
            recommendations.push_back(
                "Limited double precision support. Consider using single precision or mixed precision strategies."
            );
        }

        return recommendations;
    }

    // Getters for device capabilities
    [[nodiscard]] std::string getDeviceName() const { return device_name_; }
    [[nodiscard]] std::string getVendor() const { return vendor_; }
    [[nodiscard]] cl_ulong getGlobalMemorySize() const { return global_mem_size_; }
    [[nodiscard]] cl_uint getComputeUnits() const { return max_compute_units_; }

private:
    // Initialize GPU capabilities
    bool initialize() {
        cl_platform_id platforms[10];
        cl_uint num_platforms;
        cl_int err;

        // Get platforms
        err = clGetPlatformIDs(10, platforms, &num_platforms);
        if (err != CL_SUCCESS || num_platforms == 0) {
            std::cerr << "No OpenCL platforms found\n";
            return false;
        }

        // Attempt to find a suitable GPU
        for (cl_uint p = 0; p < num_platforms; p++) {
            char platform_vendor[128];
            clGetPlatformInfo(platforms[p], CL_PLATFORM_VENDOR, 
                              sizeof(platform_vendor), platform_vendor, NULL);

            cl_device_id devices[10];
            cl_uint num_devices;
            err = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_GPU, 
                                 10, devices, &num_devices);

            if (err == CL_SUCCESS && num_devices > 0) {
                // Use first GPU device
                cl_device_id device = devices[0];
                
                // Retrieve device information
                clGetDeviceInfo(device, CL_DEVICE_NAME, 
                                sizeof(device_name_), 
                                device_name_, NULL);
                
                clGetDeviceInfo(device, CL_DEVICE_VENDOR, 
                                sizeof(vendor_), 
                                vendor_, NULL);
                
                clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, 
                                sizeof(global_mem_size_), 
                                &global_mem_size_, NULL);
                
                clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, 
                                sizeof(max_mem_alloc_size_), 
                                &max_mem_alloc_size_, NULL);
                
                clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, 
                                sizeof(max_compute_units_), 
                                &max_compute_units_, NULL);
                
                clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, 
                                sizeof(max_work_group_size_), 
                                &max_work_group_size_, NULL);
                
                // Double Precision Support
                cl_device_fp_config fp_config;
                clGetDeviceInfo(device, CL_DEVICE_DOUBLE_FP_CONFIG, 
                                sizeof(fp_config), &fp_config, NULL);
                supports_double_precision_ = fp_config != 0;
                
                // Clock Frequency
                clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, 
                                sizeof(max_clock_frequency_), 
                                &max_clock_frequency_, NULL);
                
                // Number of Devices
                num_available_devices_ = num_devices;
                
                return true;
            }
        }

        std::cerr << "No GPU devices found\n";
        return false;
    }

    // Helper method to format memory size
    static std::string formatMemorySize(cl_ulong bytes) {
        constexpr double GB = 1024.0 * 1024.0 * 1024.0;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << (bytes / GB) << " GB";
        return oss.str();
    }

    // Device information
    char device_name_[256] = {0};
    char vendor_[128] = {0};
    
    // Computational details
    cl_ulong global_mem_size_ = 0;
    cl_ulong max_mem_alloc_size_ = 0;
    cl_uint max_compute_units_ = 0;
    cl_ulong max_work_group_size_ = 0;
    
    // Feature support flags
    bool supports_double_precision_ = false;
    bool supports_local_memory_ = false;
    bool supports_unified_memory_ = false;
    
    // Performance characteristics
    cl_ulong max_clock_frequency_ = 0;
    
    // Multi-GPU configuration
    bool supports_multi_gpu_ = false;
    cl_uint num_available_devices_ = 0;
};

// Convenience function for detecting GPU capabilities
inline std::unique_ptr<GPUDeviceCapabilities> detect_gpu_capabilities() {
    return GPUDeviceCapabilities::detect();
}

// Function to recommend GPU optimizations
inline void recommend_gpu_optimizations(const GPUDeviceCapabilities* capabilities) {
    if (!capabilities) {
        std::cerr << "No GPU capabilities detected.\n";
        return;
    }

    std::cout << "GPU Optimization Recommendations:\n"
              << "--------------------------------\n";
    
    capabilities->printDetails();

    auto recommendations = capabilities->getOptimizationRecommendations();
    if (!recommendations.empty()) {
        std::cout << "\nRecommendations:\n";
        for (const auto& rec : recommendations) {
            std::cout << "- " << rec << "\n";
        }
    }
}

#endif // GPU_OPTIMIZATION_DETECTION_H
