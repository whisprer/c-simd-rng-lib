cmake_minimum_required(VERSION 3.14)
project(enhanced_bitwidth_benchmark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(enhanced_bitwidth_benchmark ../v-0-1-5_[release_i]/enhanced_bitwidth_benchmark.cpp)

# Use absolute path to force file linking
target_link_libraries(enhanced_bitwidth_benchmark 
    "${CMAKE_CURRENT_SOURCE_DIR}/universal_rng.lib"
)

target_include_directories(enhanced_bitwidth_benchmark PRIVATE
    /d/code/Universal-Architecture-RNG-Lib/non-core-files/include/
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(enhanced_bitwidth_benchmark PRIVATE -O3 -march=native)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(enhanced_bitwidth_benchmark PRIVATE /O2 /arch:AVX2)
endif()