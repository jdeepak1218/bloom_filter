cmake_minimum_required(VERSION 3.15)
project(BloomFilter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(bloom_lib
    src/bloom_filter.cpp
    src/murmur3.cpp
)
target_include_directories(bloom_lib PUBLIC include)

add_executable(test_bloom tests/test_bloom.cpp)
target_link_libraries(test_bloom bloom_lib)
