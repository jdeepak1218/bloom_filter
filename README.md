# Bloom Filter System in C++

![Language](https://img.shields.io/badge/language-C%2B%2B17-blue)
![Build](https://img.shields.io/badge/build-CMake-orange)
![License](https://img.shields.io/badge/license-MIT-green)
![Threads](https://img.shields.io/badge/thread--safe-yes-brightgreen)
![Benchmark](https://img.shields.io/badge/benchmarked-Google%20Benchmark-yellow)

A production-grade probabilistic data structure library in C++17 featuring three filter variants, thread safety, binary persistence, and a real-world URL safety checker that identifies malicious URLs from a database of 68,867 known threats in under 0.02ms — using the same architecture as Chrome's Safe Browsing.

---

##  What is a Bloom Filter?

A Bloom Filter is a probabilistic data structure that answers one question:

> **"Have I seen this element before?"**

It answers with either:
- **"Definitely NO"** — the element was never inserted. This is always 100% accurate.
- **"Possibly YES"** — the element was probably inserted, but there is a small chance of a false positive.

### How it works

A Bloom Filter maintains a **bit array** of size `m`, all initialized to 0. It uses `k` hash functions to map each element to `k` positions in the array.

**Insertion:** Hash the element to `k` positions and set those bits to 1.

**Lookup:** Hash the element to the same `k` positions and check if ALL bits are 1.
- If ANY bit is 0 → element was **definitely never inserted**
- If ALL bits are 1 → element was **probably inserted**

```
Bit Array:  [0][0][0][0][0][0][0][0][0][0]   (initial)

Insert "apple" → hash to positions 1, 4, 7
            [0][1][0][0][1][0][0][1][0][0]

Insert "banana" → hash to positions 2, 4, 8
            [0][1][1][0][1][0][0][1][1][0]

Check "apple" → positions 1, 4, 7 → all 1 → POSSIBLY in set ✅
Check "grape" → positions 0, 3, 6 → bit 0 is 0 → DEFINITELY not in set ✅
```

### Why use a Bloom Filter?

| Property | Value |
|---|---|
| Space efficiency | Orders of magnitude smaller than hash sets |
| Lookup speed | Sub-microsecond |
| False negatives | Never — guaranteed |
| False positives | Configurable — typically 1-9% |
| Deletions | Not supported (use Counting Bloom Filter) |

### The math

Given `n` expected elements and desired false positive rate `p`:

```
m = -(n × ln(p)) / (ln(2))²     → optimal bit array size
k = (m / n) × ln(2)             → optimal number of hash functions
```

These formulas are computed automatically — you never hardcode `m` or `k`.

---

##  Architecture

This project implements three filter variants, each solving a different problem:

```
┌─────────────────────────────────────────────────────────────┐
│                    Bloom Filter System                       │
├─────────────────┬──────────────────┬────────────────────────┤
│  BloomFilter    │ CountingBloomFilter│ ScalableBloomFilter   │
│                 │                  │                        │
│ • Fixed capacity│ • Fixed capacity  │ • Unlimited capacity  │
│ • 1 bit/position│ • 8 bits/position │ • Chain of filters    │
│ • No deletion   │ • Supports delete │ • Auto grows          │
│ • 30 KB memory  │ • 244 KB memory   │ • Starts at 30 KB     │
│ • Fastest       │ • Slightly slower │ • Slightly slower      │
└─────────────────┴──────────────────┴────────────────────────┘
         │                 │                    │
         └─────────────────┴────────────────────┘
                           │
                    MurmurHash3 (128-bit)
                    Double Hashing Strategy
                    Thread Safety (shared_mutex)
                    Binary Serialization
```

---

##  Features

### Core Features
- **Three filter variants** — Regular, Counting, and Scalable
- **MurmurHash3** — production grade 128-bit hashing with double hashing strategy
- **Thread safe** — `std::shared_mutex` allows multiple concurrent readers, exclusive writers
- **Binary serialization** — save and load filters instantly from disk
- **Input validation** — proper error handling with meaningful exceptions
- **Auto-computed parameters** — `m` and `k` always computed from formulas, never hardcoded

### Bloom Filter
- Optimal bit array with `vector<bool>` (automatically bit-packed)
- Early return optimization in `contains()` — returns false on first unset bit
- Sub-millisecond lookups across 50,000 elements

### Counting Bloom Filter
- Each position stores a `uint8_t` counter instead of a single bit
- **Supports deletion** — decrement counters instead of clearing bits
- Overflow protection — counters capped at 255
- 8x more memory than regular filter but enables deletion

### Scalable Bloom Filter
- Automatically creates new larger filters when capacity is reached
- Growth factor of 2x — each new filter is twice the capacity
- Tightening ratio of 0.85 — each new filter has a tighter FPR to keep overall FPR bounded
- Queries check all filters — zero false negatives guaranteed across all inserted elements

### URL Safety Checker
- Loads 68,867 known malicious URLs from URLhaus database
- Caches filter to binary file — first run builds filter, subsequent runs load instantly
- URL normalization — strips `http://`, `https://`, converts to lowercase
- Sub-millisecond checks — identifies malicious URLs in ~21,300 ns

---

## Performance Benchmarks

Benchmarked using Google Benchmark on a 20-core CPU @ 2688 MHz with 24MB L3 cache.

### Core Operations

| Operation | Latency | Throughput |
|---|---|---|
| BloomFilter Insert | 433 ns | 1.9M ops/sec |
| BloomFilter Lookup (hit) | 303 ns | 3.3M ops/sec |
| BloomFilter Lookup (miss) | 299 ns | 3.6M ops/sec |
| CountingFilter Insert | 430 ns | 1.9M ops/sec |
| CountingFilter Lookup | 291 ns | 3.4M ops/sec |
| CountingFilter Remove | 566 ns | 1.8M ops/sec |
| ScalableFilter Insert | 510 ns | 1.9M ops/sec |
| ScalableFilter Lookup | 317 ns | 3.1M ops/sec |

### Thread Scaling (shared_mutex)

| Threads | Latency | Throughput |
|---|---|---|
| 1 thread | 294 ns | 3.4M ops/sec |
| 2 threads | 255 ns | 8.0M ops/sec |
| 4 threads | 472 ns | 8.8M ops/sec |
| 8 threads | 1120 ns | 7.9M ops/sec |

Two threads gives **2.3x speedup** — proving `shared_mutex` allows concurrent reads correctly.

### Filter Size Scaling

| Filter Size (n) | Latency | Throughput |
|---|---|---|
| 1,000 elements | 243 ns | 3.9M ops/sec |
| 10,000 elements | 243 ns | 4.1M ops/sec |
| 50,000 elements | 255 ns | 4.0M ops/sec |
| 100,000 elements | 249 ns | 4.3M ops/sec |

Performance stays flat across all sizes because even the largest filter (100,000 elements ≈ 60KB) fits comfortably in the 24MB L3 cache.

---

##  URL Safety Checker Demo

```
$ ./url_checker "http://s4.quicklyspeed.com/linux_arm7"

Loaded 68,867 malicious URLs in 517ms
Filter cached to data/malicious_filter.bin

URL checked: s4.quicklyspeed.com/linux_arm7

WARNING - possible malicious URL detected!
This URL may be in the malicious database.
Note: 0.9% chance of false positive.

=== Stats ===
Check time:  21,300 ns
Filter FPR:  0.906611%
Filter size: 81 KB
```

```
$ ./url_checker "google.com"

Filter loaded from cache in 2ms

URL checked: google.com

SAFE - not found in malicious database.

=== Stats ===
Check time:  18,400 ns
Filter FPR:  0.906611%
Filter size: 81 KB
```

---

## Test Results

```
========================================
         BLOOM FILTER TESTS
========================================
Inserted 50,000 words in 84ms
Zero False Negatives:  PASSED
False Positive Rate:   9.19% (expected 9%)
Serialize/Deserialize: PASSED

========================================
    COUNTING BLOOM FILTER TESTS
========================================
Inserted 50,000 words in 63ms
Zero False Negatives:  PASSED
Deletion Test:         PASSED
False Positive Rate:   9.19% (expected 9%)
Serialize/Deserialize: PASSED

========================================
    SCALABLE BLOOM FILTER TESTS
========================================
Inserted 150,000 words (3x initial capacity)
Auto Scaling:          PASSED (1 → 3 filters)
Zero False Negatives:  PASSED (all 150,000 found)
False Positive Rate:   10.08%
Serialize/Deserialize: PASSED
```

---

##  Real World Applications

Bloom Filters are used everywhere in production systems:

### Google Bigtable
Uses Bloom Filters to avoid expensive disk lookups. Before reading from disk it checks the filter — if the filter says "definitely not here" it skips the disk read entirely. Saves millions of unnecessary I/O operations per second.

### Apache Cassandra
Used by Netflix, Instagram, and Discord. Cassandra stores one Bloom Filter per SSTable (data file) on disk. When querying a row, it checks each filter before reading each file — dramatically reducing I/O.

### Chrome Safe Browsing
Chrome downloads a Bloom Filter of known malicious URLs. Every URL you visit is checked against the local filter first. If the filter says "possibly malicious" it sends a request to Google's servers for verification. This project implements exactly this architecture.

### Redis
RedisBloom module uses Scalable Bloom Filters — exactly what this project implements — to handle datasets that grow over time without degrading false positive rate.

### Bitcoin
Bitcoin nodes use Bloom Filters to efficiently sync transactions between peers without downloading the entire blockchain.

---

## Hashing Strategy

This project uses **MurmurHash3** exclusively — specifically `MurmurHash3_x64_128` which produces 128-bit output.

The 128-bit output is split into two 64-bit values `h1` and `h2`, then **double hashing** simulates `k` independent hash functions from a single hash call:

```cpp
uint64_t hashes[2];
MurmurHash3_x64_128(key.c_str(), key.size(), 42, hashes);
uint64_t h1 = hashes[0];
uint64_t h2 = hashes[1];

for (int i = 0; i < k; i++) {
    uint64_t pos = (h1 + i * h2) % m;
    // set or check bit at pos
}
```

One hash call → k independent bit positions. Proven to have the same asymptotic behavior as k truly independent hash functions while being significantly faster.

---

##  Thread Safety

All three filter variants use `std::shared_mutex` from C++17:

```
Multiple threads calling contains() simultaneously → shared_lock  ✅ allowed
One thread calling add() while others read          → unique_lock ✅ safe
Two threads calling add() simultaneously            → unique_lock ✅ serialized
```

This is the readers-writer pattern used in production databases. Benchmark results show 2.3x throughput improvement with 2 concurrent reader threads.

---

##  Project Structure

```
bloom_filter/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── include/
│   ├── bloom_filter.h
│   ├── counting_bloom_filter.h
│   ├── scalable_bloom_filter.h
│   └── murmur3.h
├── src/
│   ├── bloom_filter.cpp
│   ├── counting_bloom_filter.cpp
│   ├── scalable_bloom_filter.cpp
│   ├── url_checker.cpp
│   └── murmur3.cpp
├── tests/
│   └── test_bloom.cpp
├── benchmarks/
│   └── bench.cpp
└── data/
    ├── words.txt
    └── malicious_urls.txt
```

---

##  How to Build and Run

### Prerequisites
- CMake 3.15+
- C++17 compatible compiler (GCC, Clang, MSVC)
- vcpkg (for Google Benchmark)

### Build

```bash
git clone https://github.com/jdeepak1218/bloom_filter.git
cd bloom_filter
mkdir build && cd build
cmake ..
cmake --build .
```

### Run Tests

```bash
# Windows
.\build\Debug\test_bloom.exe

# Linux/Mac
./build/test_bloom
```

### Run URL Safety Checker

```bash
# Windows
.\build\Debug\url_checker.exe "http://suspicious-site.com"

# Linux/Mac
./build/url_checker "http://suspicious-site.com"
```

### Run Benchmarks

```bash
# Must build in Release for accurate results
cmake --build . --config Release

# Windows
.\build\Release\bench.exe

# Linux/Mac
./build/bench
```

### Build with Google Benchmark (Windows/vcpkg)

```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake \
         -DVCPKG_TARGET_TRIPLET=x64-windows \
         -Dbenchmark_DIR=C:\vcpkg\installed\x64-windows\share\benchmark
cmake --build . --config Release
```

---

##  Future Work

- **d-left Counting Bloom Filter** — 50% memory reduction over regular Counting Bloom Filter using d-left hashing and 4-bit counters
- **Invertible Bloom Filter** — supports listing all elements and computing set differences between two filters. Used in Bitcoin transaction syncing
- **Memory mapped I/O** — replace serialize/deserialize with `mmap` for instant filter loading regardless of size
- **SIMD optimizations** — use CPU vector instructions to check multiple bit positions simultaneously, potentially 4-8x speedup
- **Interactive Visualizer** — web-based visualization showing bit array updates in real time as elements are inserted *(coming soon)*

---

##  References

- [Space/Time Trade-offs in Hash Coding with Allowable Errors](https://dl.acm.org/doi/10.1145/362686.362692) — Burton Bloom's original 1970 paper
- [Scalable Bloom Filters](https://www.sciencedirect.com/science/article/pii/S0020019006003127) — Almeida et al., introduced Scalable Bloom Filter
- [MurmurHash3](https://github.com/aappleby/smhasher) — Austin Appleby's reference implementation
- [URLhaus](https://urlhaus.abuse.ch) — Malicious URL database used in this project

---

##  License

MIT License — feel free to use this code in your own projects.

---

---

> 📝 **Transparency Note:** This README was written with the assistance of [Claude](https://claude.ai) (Anthropic's AI assistant). The code, architecture decisions, implementation, debugging, and all technical work was done by the developer. Claude was used as a teaching and writing aid throughout the project.
