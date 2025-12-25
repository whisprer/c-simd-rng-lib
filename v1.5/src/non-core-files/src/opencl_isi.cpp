#ifndef UNIVERSAL_RNG_OPENCL_H
#define UNIVERSAL_RNG_OPENCL_H

#ifdef __APPLE__
#include <OpenCL/opencl.h>
// #else
// #include <CL/cl.h>
// #endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Precision mode enum
typedef enum {
    RNG_SINGLE_PRECISION,
    RNG_DOUBLE_PRECISION,
    RNG_MIXED_PRECISION
} rng_precision_mode;

// OpenCL RNG state structure
typedef struct {
    cl_context       context;
    cl_command_queue command_queue;
    cl_program       program;
    cl_kernel        init_kernel;
    cl_kernel        generate_kernel;
    cl_mem           state_buffer;
    cl_mem           result_buffer;
    
    size_t           batch_size;
    size_t           work_group_size;
    rng_precision_mode precision;
    
    void*            host_results;
    size_t           current_pos;
    int              is_initialized;
} opencl_rng_state;

// OpenCL kernel source (note: this is a simplified version)
static const char* rng_kernel_source = R"(
// WyRand core implementation in OpenCL kernel
__kernel void wyrand_init(
    __global ulong* states, 
    ulong seed, 
    uint num_streams
) {
    uint gid = get_global_id(0);
    if (gid < num_streams) {
        // Use SplitMix64 for seeding
        ulong z = seed + ((ulong)gid * 0x9e3779b97f4a7c15UL);
        z ^= (z >> 30);
        z *= 0xbf58476d1ce4e5b9UL;
        z ^= (z >> 27);
        z *= 0x94d049bb133111ebUL;
        states[gid] = z ^ (z >> 31);
    }
}

__kernel void wyrand_generate(
    __global ulong* states, 
    __global ulong* results, 
    uint num_streams
) {
    uint gid = get_global_id(0);
    if (gid < num_streams) {
        ulong seed = states[gid];
        
        // WyRand core generation
        seed += 0xa0761d6478bd642FULL;
        ulong result = seed ^ (seed >> 32);
        
        // Store results and updated state
        results[gid] = result;
        states[gid] = seed;
    }
}
)";

// Error handling macro
#define CHECK_CL_ERROR(err, msg) \
    if (err != CL_SUCCESS) { \
        fprintf(stderr, "OpenCL Error (%d): %s\n", err, msg); \
        return NULL; \
    }

// Function to select the best NVIDIA or Intel device
cl_device_id select_best_device() {
    cl_platform_id platforms[10];
    cl_uint num_platforms;
    cl_int err = clGetPlatformIDs(10, platforms, &num_platforms);
    
    if (err != CL_SUCCESS || num_platforms == 0) {
        fprintf(stderr, "No OpenCL platforms found\n");
        return NULL;
    }

    for (cl_uint p = 0; p < num_platforms; p++) {
        char platform_vendor[128];
        err = clGetPlatformInfo(platforms[p], CL_PLATFORM_VENDOR, 
                                sizeof(platform_vendor), platform_vendor, NULL);
        
        // Prioritize NVIDIA, then Intel
        if (strstr(platform_vendor, "NVIDIA") || strstr(platform_vendor, "Intel")) {
            cl_device_id devices[10];
            cl_uint num_devices;
            
            err = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_GPU, 
                                 10, devices, &num_devices);
            
            if (err == CL_SUCCESS && num_devices > 0) {
                // Return first GPU device
                return devices[0];
            }
        }
    }
    
    return NULL;
}

// Create OpenCL RNG state
opencl_rng_state* create_opencl_rng(
    uint64_t seed, 
    size_t batch_size, 
    rng_precision_mode precision
) {
    cl_int err;
    
    // Allocate state
    opencl_rng_state* state = malloc(sizeof(opencl_rng_state));
    if (!state) return NULL;
    
    // Select device
    cl_device_id device = select_best_device();
    if (!device) {
        free(state);
        return NULL;
    }
    
    // Create context
    state->context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CHECK_CL_ERROR(err, "Failed to create OpenCL context");
    
    // Create command queue
    state->command_queue = clCreateCommandQueue(
        state->context, device, 0, &err
    );
    CHECK_CL_ERROR(err, "Failed to create command queue");
    
    // Create program
    const char* source = rng_kernel_source;
    state->program = clCreateProgramWithSource(
        state->context, 1, &source, NULL, &err
    );
    CHECK_CL_ERROR(err, "Failed to create program");
    
    // Build program
    err = clBuildProgram(state->program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        size_t log_size;
        clGetProgramBuildInfo(
            state->program, device, CL_PROGRAM_BUILD_LOG, 
            0, NULL, &log_size
        );
        
        char* log = malloc(log_size);
        clGetProgramBuildInfo(
            state->program, device, CL_PROGRAM_BUILD_LOG, 
            log_size, log, NULL
        );
        
        fprintf(stderr, "Program build log: %s\n", log);
        free(log);
        
        return NULL;
    }
    
    // Create kernels
    state->init_kernel = clCreateKernel(state->program, "wyrand_init", &err);
    CHECK_CL_ERROR(err, "Failed to create init kernel");
    
    state->generate_kernel = clCreateKernel(state->program, "wyrand_generate", &err);
    CHECK_CL_ERROR(err, "Failed to create generate kernel");
    
    // Set batch parameters
    state->batch_size = batch_size;
    state->precision = precision;
    
    // Allocate buffers
    state->state_buffer = clCreateBuffer(
        state->context, 
        CL_MEM_READ_WRITE, 
        batch_size * sizeof(uint64_t), 
        NULL, &err
    );
    CHECK_CL_ERROR(err, "Failed to create state buffer");
    
    state->result_buffer = clCreateBuffer(
        state->context, 
        CL_MEM_WRITE_ONLY, 
        batch_size * sizeof(uint64_t), 
        NULL, &err
    );
    CHECK_CL_ERROR(err, "Failed to create result buffer");
    
    // Allocate host results
    state->host_results = malloc(batch_size * sizeof(uint64_t));
    state->current_pos = batch_size;  // Force first generation
    
    // Initialize states
    size_t global_work_size = batch_size;
    err = clSetKernelArg(state->init_kernel, 0, sizeof(cl_mem), &state->state_buffer);
    err |= clSetKernelArg(state->init_kernel, 1, sizeof(uint64_t), &seed);
    err |= clSetKernelArg(state->init_kernel, 2, sizeof(uint32_t), &global_work_size);
    
    err |= clEnqueueNDRangeKernel(
        state->command_queue, state->init_kernel, 1, 
        NULL, &global_work_size, NULL, 0, NULL, NULL
    );
    
    state->is_initialized = 1;
    
    return state;
}

// Generate next batch of random numbers
uint64_t* generate_next_batch(opencl_rng_state* state) {
    if (!state || !state->is_initialized) return NULL;
    
    cl_int err;
    size_t global_work_size = state->batch_size;
    
    // Set kernel arguments
    err = clSetKernelArg(state->generate_kernel, 0, sizeof(cl_mem), &state->state_buffer);
    err |= clSetKernelArg(state->generate_kernel, 1, sizeof(cl_mem), &state->result_buffer);
    err |= clSetKernelArg(state->generate_kernel, 2, sizeof(uint32_t), &global_work_size);
    
    // Enqueue kernel
    err |= clEnqueueNDRangeKernel(
        state->command_queue, state->generate_kernel, 1, 
        NULL, &global_work_size, NULL, 0, NULL, NULL
    );
    
    // Read results
    err |= clEnqueueReadBuffer(
        state->command_queue, state->result_buffer, CL_TRUE, 0,
        state->batch_size * sizeof(uint64_t), 
        state->host_results, 0, NULL, NULL
    );
    
    state->current_pos = 0;
    
    return (uint64_t*)state->host_results;
}

// Free OpenCL RNG resources
void free_opencl_rng(opencl_rng_state* state) {
    if (!state) return;
    
    if (state->state_buffer) clReleaseMemObject(state->state_buffer);
    if (state->result_buffer) clReleaseMemObject(state->result_buffer);
    if (state->init_kernel) clReleaseKernel(state->init_kernel);
    if (state->generate_kernel) clReleaseKernel(state->generate_kernel);
    if (state->program) clReleaseProgram(state->program);
    if (state->command_queue) clReleaseCommandQueue(state->command_queue);
    if (state->context) clReleaseContext(state->context);
    
    free(state->host_results);
    free(state);
}
#endif

#endif // UNIVERSAL_RNG_OPENCL_H
