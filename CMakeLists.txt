cmake_minimum_required(VERSION 3.15)
project(BloomFilter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(bloom_lib
    src/bloom_filter.cpp
    src/murmur3.cpp
    src/counting_bloom_filter.cpp
    src/scalable_bloom_filter.cpp
)
target_include_directories(bloom_lib PUBLIC include)
add_executable(url_checker src/url_checker.cpp)
target_link_libraries(url_checker bloom_lib)
add_executable(test_bloom tests/test_bloom.cpp)
target_link_libraries(test_bloom bloom_lib)

find_package(benchmark CONFIG REQUIRED)
add_executable(bench benchmarks/bench.cpp)
target_link_libraries(bench bloom_lib benchmark::benchmark)
