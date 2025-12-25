cmake_minimum_required(VERSION 3.20)
project(universal_rng LANGUAGES CXX)

option(ENABLE_AVX2 "Enable AVX2 SIMD" ON)
option(ENABLE_AVX512 "Enable AVX-512 SIMD" ON)
option(ENABLE_BENCH "Build benchmark harness" OFF)
option(ENABLE_NEON "Enable NEON (ARM/Apple)" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

if (MSVC)
  add_compile_options(/O2 /permissive- /Zc:preprocessor /DNOMINMAX)
  if (ENABLE_AVX2)
    add_compile_options(/arch:AVX2)
  endif()
else()
  add_compile_options(-O3 -fno-exceptions -fno-rtti -fno-math-errno -fno-trapping-math)
  if (ENABLE_AVX2)
    add_compile_options(-mavx2 -mbmi2)
  endif()
  if (ENABLE_AVX512)
    add_compile_options(-mavx512f -mavx512vl -mavx512dq)
  endif()
  if (ENABLE_NEON AND (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM|arm64"))
    add_compile_options(-march=armv8-a+simd)
  endif()
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(universal_rng
  src/universal_rng.cpp
  src/runtime_detect.cpp
  # xoshiro
  src/xoshiro256ss_scalar.cpp
  src/simd_avx2_xoshiro256ss.cpp
  src/simd_avx512_xoshiro256ss.cpp
  # philox
  src/philox4x32_scalar.cpp
  src/simd_avx2_philox4x32.cpp
  src/simd_avx512_philox4x32.cpp
  src/simd_neon_philox4x32.cpp
  # distributions
  src/rng_distributions.cpp
  # NEW: SIMD Ziggurat normals
  src/simd_normal_avx2.cpp
  src/simd_normal_avx512.cpp
  src/simd_normal_neon.cpp
)

target_include_directories(universal_rng PUBLIC include)
target_compile_definitions(universal_rng PRIVATE $<$<BOOL:${ENABLE_AVX2}>:UA_ENABLE_AVX2>)
target_compile_definitions(universal_rng PRIVATE $<$<BOOL:${ENABLE_AVX512}>:UA_ENABLE_AVX512>)

if (ENABLE_BENCH)
  add_executable(rng_selftest bench/bench_main.cpp)
  target_link_libraries(rng_selftest universal_rng)

  add_executable(rng_lockbench bench/bench_lock_and_bench.cpp)
  target_link_libraries(rng_lockbench universal_rng)
endif()
