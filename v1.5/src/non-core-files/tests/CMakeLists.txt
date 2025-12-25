add_executable(rng_selftest test_rng.cpp)
target_link_libraries(rng_selftest PRIVATE universal_rng)

add_test(NAME rng_selftest COMMAND rng_selftest)

