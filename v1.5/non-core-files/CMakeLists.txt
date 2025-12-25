cmake_minimum_required(VERSION 3.14)
project(universal_rng LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force shared library build
set(BUILD_SHARED_LIBS ON)

# Source files
set(SRC_FILES
    src/universal_rng.cpp
    src/universal_rng_api.cpp
    src/universal_rng_bitwidth.cpp
    src/universal_rng_scalar.cpp
    src/runtime_detect.cpp
    src/cpu_detect.cpp
    src/simd_sse2.cpp
    src/simd_avx2.cpp
)

# Create shared library
add_library(universal_rng SHARED ${SRC_FILES})

# Force enable AVX2 and SSE2
target_compile_definitions(universal_rng PRIVATE USE_SSE2 USE_AVX2)

# Platform-specific settings
if(WIN32)
    message(STATUS "Configuring for Windows (.dll)")
    
    # MSVC flags
    if(MSVC)
        set_source_files_properties(src/simd_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
        target_compile_options(universal_rng PRIVATE /O2)
    endif()
    
    # MinGW flags  
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set_source_files_properties(src/simd_sse2.cpp PROPERTIES COMPILE_FLAGS "-msse2")
        set_source_files_properties(src/simd_avx2.cpp PROPERTIES COMPILE_FLAGS "-mavx2")
        target_compile_options(universal_rng PRIVATE -O3)
    endif()
    
    # Windows DLL export - use simple approach
    set_target_properties(universal_rng PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
    
else()
    message(STATUS "Configuring for Linux (.so)")
    
    # GCC/Clang flags for Linux
    set_source_files_properties(src/simd_sse2.cpp PROPERTIES COMPILE_FLAGS "-msse2")
    set_source_files_properties(src/simd_avx2.cpp PROPERTIES COMPILE_FLAGS "-mavx2")
    target_compile_options(universal_rng PRIVATE 
        -O3 
        -march=native 
        -fPIC  # Position Independent Code for shared libraries
    )
    
    # Set version and soname for Linux
    set_target_properties(universal_rng PROPERTIES
        VERSION 1.0.0
        SOVERSION 1
    )
endif()

# Include directories
target_include_directories(universal_rng PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Test executable
add_executable(rng_selftest tests/test_rng.cpp)
target_link_libraries(rng_selftest universal_rng)

# Install rules
install(TARGETS universal_rng 
    LIBRARY DESTINATION lib    # .so files
    ARCHIVE DESTINATION lib    # .a files  
    RUNTIME DESTINATION bin    # .dll files
)

install(DIRECTORY include/ DESTINATION include)

message(STATUS "=== Configuration ===")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Building: SHARED library")
message(STATUS "AVX2: FORCED ON")
message(STATUS "====================")