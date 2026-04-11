#include <benchmark/benchmark.h>
#include "bloom_filter.h"
#include "counting_bloom_filter.h"
#include "scalable_bloom_filter.h"
#include <string>
#include <vector>
#include <fstream>
static std::vector<std::string> loadWords(size_t count = 60000) {
    std::vector<std::string> words;
    std::ifstream file("data/words.txt");
    std::string word;
    while (file >> word && words.size() < count) {
        words.push_back(word);
    }
    return words;
}
static const std::vector<std::string> WORDS = loadWords(60000);
static void BM_BloomFilter_Insert(benchmark::State& state) {
    BloomFilter bf(50000, 0.09);
    size_t i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        const std::string& word = WORDS[i % 50000];
        state.ResumeTiming();
        bf.add(word);
        benchmark::ClobberMemory();
        i++;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_BloomFilter_Insert);
static void BM_BloomFilter_Contains_Hit(benchmark::State& state) {
    BloomFilter bf(50000, 0.09);
    for (size_t i = 0; i < 50000; i++) bf.add(WORDS[i]);
    size_t i = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(bf.contains(WORDS[i % 50000]));
        i++;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_BloomFilter_Contains_Hit);
static void BM_BloomFilter_Contains_Miss(benchmark::State& state) {
    BloomFilter bf(50000, 0.09);
    for (size_t i = 0; i < 50000; i++) bf.add(WORDS[i]);
    size_t i = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(bf.contains(WORDS[50000 + (i % 10000)]));
        i++;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_BloomFilter_Contains_Miss);
static void BM_CountingBloomFilter_Insert(benchmark::State& state) {
    CountingBloomFilter cbf(50000, 0.09);
    size_t i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        const std::string& word = WORDS[i % 50000];
        state.ResumeTiming();
        cbf.add(word);
        benchmark::ClobberMemory();
        i++;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CountingBloomFilter_Insert);
static void BM_CountingBloomFilter_Contains(benchmark::State& state) {
    CountingBloomFilter cbf(50000, 0.09);
    for (size_t i = 0; i < 50000; i++) cbf.add(WORDS[i]);
    size_t i = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(cbf.contains(WORDS[i % 50000]));
        i++;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CountingBloomFilter_Contains);
static void BM_CountingBloomFilter_Remove(benchmark::State& state) {
    CountingBloomFilter cbf(50000, 0.09);
    for (size_t i = 0; i < 50000; i++) cbf.add(WORDS[i]);
    size_t i = 0;
    for (auto _ : state) {
        // add and remove same word — keeps counter stable
        cbf.add(WORDS[i % 50000]);
        cbf.remove(WORDS[i % 50000]);
        benchmark::ClobberMemory();
        i++;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CountingBloomFilter_Remove);
static void BM_ScalableBloomFilter_Insert(benchmark::State& state) {
    ScalableBloomFilter sbf(50000, 0.09);
    size_t i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        const std::string& word = WORDS[i % 50000];
        state.ResumeTiming();
        sbf.add(word);
        benchmark::ClobberMemory();
        i++;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ScalableBloomFilter_Insert);
static void BM_ScalableBloomFilter_Contains(benchmark::State& state) {
    ScalableBloomFilter sbf(50000, 0.09);
    for (size_t i = 0; i < 50000; i++) sbf.add(WORDS[i]);
    size_t i = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(sbf.contains(WORDS[i % 50000]));
        i++;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ScalableBloomFilter_Contains);
static void BM_BloomFilter_Contains_MultiThread(benchmark::State& state) {
    static BloomFilter bf(50000, 0.09);
    static bool initialized = false;
    if (!initialized) {
        for (size_t i = 0; i < 50000; i++) bf.add(WORDS[i]);
        initialized = true;
    }
    size_t i = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(bf.contains(WORDS[i % 50000]));
        i++;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_BloomFilter_Contains_MultiThread)->Threads(1)->Threads(2)->Threads(4)->Threads(8);
static void BM_BloomFilter_Insert_Range(benchmark::State& state) {
    size_t n = state.range(0);
    BloomFilter bf(n, 0.09);
    size_t i = 0;
    for (auto _ : state) {
        state.PauseTiming();
        const std::string& word = WORDS[i % std::min(n, WORDS.size())];
        state.ResumeTiming();
        bf.add(word);
        benchmark::ClobberMemory();
        i++;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_BloomFilter_Insert_Range)->Arg(1000)->Arg(10000)->Arg(50000)->Arg(100000);
BENCHMARK_MAIN();
