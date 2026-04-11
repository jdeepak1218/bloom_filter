#include <benchmark/benchmark.h>
#include "bloom_filter.h"
#include "counting_bloom_filter.h"
#include <string>
static void BloomFilter_insert(benchmark::State &state){
  BloomFilter bf(50000,0.09);
  for(auto _ : state){
    bf.add("apple");
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BloomFilter_insert);
static void CountingBloomFilter_insert(benchmark::State &state){
  CountingBloomFilter cb(50000,0.09);
  for(auto _ : state){
    cb.add("apple");
    benchmark::ClobberMemory();
  }
}
BENCHMARK(CountingBloomFilter_insert);
static void BloomFilter_contains(benchmark::State &state)
{
  BloomFilter bf(50000,0.09);
  bf.add("apple");
  for(auto _ : state){
    benchmark::DoNotOptimize(bf.contains("apple"));
  }
}
BENCHMARK(BloomFilter_contains);
static void CountingBloomFilter_contains(benchmark::State &state)
{
  CountingBloomFilter cb(50000,0.09);
  cb.add("apple");
  for(auto _ : state){
    benchmark::DoNotOptimize(cb.contains("apple"));
  }
}
BENCHMARK(CountingBloomFilter_contains);
BENCHMARK_MAIN();
