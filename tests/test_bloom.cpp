#include "bloom_filter.h"
#include "counting_bloom_filter.h"
#include "scalable_bloom_filter.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>

int main() {
    // ─── Load words ───────────────────────────────────────────────────────
    std::vector<std::string> words;
    std::ifstream wordFile("data/words.txt");
    if (!wordFile) {
        std::cerr << "Could not open data/words.txt\n";
        return 1;
    }
    std::string word;
    while (wordFile >> word && words.size() < 200000) {
        words.push_back(word);
    }
    wordFile.close();

    if (words.size() < 200000) {
        std::cerr << "Not enough words. Need 200000, got " << words.size() << "\n";
        return 1;
    }

    // =========================================
    //         BLOOM FILTER TESTS
    // =========================================
    std::cout << "\n========================================\n";
    std::cout << "         BLOOM FILTER TESTS\n";
    std::cout << "========================================\n\n";

    BloomFilter bf(50000, 0.09);

    auto insertStart = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < 50000; i++) bf.add(words[i]);
    auto insertEnd = std::chrono::high_resolution_clock::now();
    double insertTime = std::chrono::duration<double, std::milli>(insertEnd - insertStart).count();

    std::cout << "=== Insertion ===\n";
    std::cout << "Inserted 50,000 words in " << insertTime << " ms\n\n";

    std::cout << "=== Zero False Negatives Test ===\n";
    bool passed = true;
    for (size_t i = 0; i < 50000; i++) {
        if (!bf.contains(words[i])) {
            std::cout << "FAILED - word not found: " << words[i] << "\n";
            passed = false;
            break;
        }
    }
    if (passed) std::cout << "PASSED - all inserted words found\n\n";

    std::cout << "=== False Positive Rate Test ===\n";
    auto queryStart = std::chrono::high_resolution_clock::now();
    size_t falsePositives = 0;
    for (size_t i = 50000; i < 60000; i++) {
        if (bf.contains(words[i])) falsePositives++;
    }
    auto queryEnd = std::chrono::high_resolution_clock::now();
    double queryTime = std::chrono::duration<double, std::milli>(queryEnd - queryStart).count();

    std::cout << "False positives: " << falsePositives << " / 10000\n";
    std::cout << "Measured FPR:    " << (static_cast<double>(falsePositives) / 10000.0) * 100 << "%\n";
    std::cout << "Expected FPR:    9%\n";
    std::cout << "Queried 10,000 words in " << queryTime << " ms\n\n";

    std::cout << "=== Serialize / Deserialize Test ===\n";
    bf.serialize("bloom.bin");
    BloomFilter bf2(50000, 0.09);
    bf2.deserialize("bloom.bin");
    bool serializePassed = true;
    for (size_t i = 0; i < 50000; i++) {
        if (!bf2.contains(words[i])) {
            std::cout << "FAILED - word not found after deserialize: " << words[i] << "\n";
            serializePassed = false;
            break;
        }
    }
    if (serializePassed) std::cout << "PASSED - all words found after deserialize\n\n";

    std::cout << "=== Filter Stats ===\n";
    std::cout << "m (bit array size): " << bf.getM() << " bits (" << bf.getM() / 8 / 1024 << " KB)\n";
    std::cout << "k (hash functions): " << bf.getK() << "\n";
    std::cout << "Current FPR:        " << bf.currentFPR() * 100 << "%\n";
    std::cout << "Insert time:        " << insertTime << " ms\n";
    std::cout << "Query time:         " << queryTime << " ms\n";

    // =========================================
    //      COUNTING BLOOM FILTER TESTS
    // =========================================
    std::cout << "\n========================================\n";
    std::cout << "    COUNTING BLOOM FILTER TESTS\n";
    std::cout << "========================================\n\n";

    CountingBloomFilter cbf(50000, 0.09);

    auto cbfInsertStart = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < 50000; i++) cbf.add(words[i]);
    auto cbfInsertEnd = std::chrono::high_resolution_clock::now();
    double cbfInsertTime = std::chrono::duration<double, std::milli>(cbfInsertEnd - cbfInsertStart).count();

    std::cout << "=== Insertion ===\n";
    std::cout << "Inserted 50,000 words in " << cbfInsertTime << " ms\n\n";

    std::cout << "=== Zero False Negatives Test ===\n";
    bool cbfPassed = true;
    for (size_t i = 0; i < 50000; i++) {
        if (!cbf.contains(words[i])) {
            std::cout << "FAILED - word not found: " << words[i] << "\n";
            cbfPassed = false;
            break;
        }
    }
    if (cbfPassed) std::cout << "PASSED - all inserted words found\n\n";

    std::cout << "=== Deletion Test ===\n";
    std::vector<std::string> testWords = { words[0], words[1], words[2], words[3], words[4] };
    bool allFoundBefore = true;
    for (auto& w : testWords) {
        if (!cbf.contains(w)) { allFoundBefore = false; break; }
    }
    if (allFoundBefore) std::cout << "Before deletion - all 5 words found\n";
    for (auto& w : testWords) cbf.remove(w);
    bool allRemovedAfter = true;
    for (auto& w : testWords) {
        if (cbf.contains(w)) { allRemovedAfter = false; }
    }
    if (allRemovedAfter) std::cout << "After deletion  - all 5 words correctly removed\n";
    else std::cout << "Note: some words still found - expected behavior due to shared counters\n";
    bool othersIntact = true;
    for (size_t i = 5; i < 100; i++) {
        if (!cbf.contains(words[i])) { othersIntact = false; break; }
    }
    if (othersIntact) std::cout << "Other words still intact after deletion\n\n";

    std::cout << "=== False Positive Rate Test ===\n";
    auto cbfQueryStart = std::chrono::high_resolution_clock::now();
    size_t cbfFalsePositives = 0;
    for (size_t i = 50000; i < 60000; i++) {
        if (cbf.contains(words[i])) cbfFalsePositives++;
    }
    auto cbfQueryEnd = std::chrono::high_resolution_clock::now();
    double cbfQueryTime = std::chrono::duration<double, std::milli>(cbfQueryEnd - cbfQueryStart).count();
    std::cout << "False positives: " << cbfFalsePositives << " / 10000\n";
    std::cout << "Measured FPR:    " << (static_cast<double>(cbfFalsePositives) / 10000.0) * 100 << "%\n";
    std::cout << "Expected FPR:    9%\n";
    std::cout << "Queried 10,000 words in " << cbfQueryTime << " ms\n\n";

    std::cout << "=== Serialize / Deserialize Test ===\n";
    cbf.serialize("counting_bloom.bin");
    CountingBloomFilter cbf2(50000, 0.09);
    cbf2.deserialize("counting_bloom.bin");
    bool cbfSerializePassed = true;
    for (size_t i = 5; i < 50000; i++) {
        if (!cbf2.contains(words[i])) {
            std::cout << "FAILED - word not found after deserialize: " << words[i] << "\n";
            cbfSerializePassed = false;
            break;
        }
    }
    if (cbfSerializePassed) std::cout << "PASSED - all words found after deserialize\n\n";

    std::cout << "=== Counting Filter Stats ===\n";
    std::cout << "m (array size):     " << cbf.getM() << " counters\n";
    std::cout << "k (hash functions): " << cbf.getK() << "\n";
    std::cout << "Current FPR:        " << cbf.currentFPR() * 100 << "%\n";
    std::cout << "Insert time:        " << cbfInsertTime << " ms\n";
    std::cout << "Query time:         " << cbfQueryTime << " ms\n";
    std::cout << "Memory usage:       " << cbf.getM() / 1024 << " KB\n";

    // =========================================
    //      SCALABLE BLOOM FILTER TESTS
    // =========================================
    std::cout << "\n========================================\n";
    std::cout << "    SCALABLE BLOOM FILTER TESTS\n";
    std::cout << "========================================\n\n";

    // start with capacity 50000 but insert 150000 words
    ScalableBloomFilter sbf(50000, 0.09);

    std::cout << "=== Auto Scaling Test ===\n";
    std::cout << "Initial filter count: " << sbf.currentFilterCount() << "\n";

    auto sbfInsertStart = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < 150000; i++) sbf.add(words[i]);
    auto sbfInsertEnd = std::chrono::high_resolution_clock::now();
    double sbfInsertTime = std::chrono::duration<double, std::milli>(sbfInsertEnd - sbfInsertStart).count();

    std::cout << "After inserting 150,000 words:\n";
    std::cout << "Filter count: " << sbf.currentFilterCount() << " (should be > 1)\n";
    if (sbf.currentFilterCount() > 1)
        std::cout << "PASSED - filter scaled automatically\n\n";
    else
        std::cout << "FAILED - filter did not scale\n\n";

    std::cout << "=== Zero False Negatives Test ===\n";
    bool sbfPassed = true;
    for (size_t i = 0; i < 150000; i++) {
        if (!sbf.contains(words[i])) {
            std::cout << "FAILED - word not found: " << words[i] << "\n";
            sbfPassed = false;
            break;
        }
    }
    if (sbfPassed) std::cout << "PASSED - all 150,000 inserted words found\n\n";

    std::cout << "=== False Positive Rate Test ===\n";
    auto sbfQueryStart = std::chrono::high_resolution_clock::now();
    size_t sbfFalsePositives = 0;
    for (size_t i = 150000; i < 160000; i++) {
        if (sbf.contains(words[i])) sbfFalsePositives++;
    }
    auto sbfQueryEnd = std::chrono::high_resolution_clock::now();
    double sbfQueryTime = std::chrono::duration<double, std::milli>(sbfQueryEnd - sbfQueryStart).count();
    std::cout << "False positives: " << sbfFalsePositives << " / 10000\n";
    std::cout << "Measured FPR:    " << (static_cast<double>(sbfFalsePositives) / 10000.0) * 100 << "%\n";
    std::cout << "Queried 10,000 words in " << sbfQueryTime << " ms\n\n";

    std::cout << "=== Serialize / Deserialize Test ===\n";
    sbf.serialize("scalable_bloom.bin");
    std::cout << "Filter saved to scalable_bloom.bin\n";
    ScalableBloomFilter sbf2(50000, 0.09);
    sbf2.deserialize("scalable_bloom.bin");
    std::cout << "Filter loaded from scalable_bloom.bin\n";
    bool sbfSerializePassed = true;
    for (size_t i = 0; i < 150000; i++) {
        if (!sbf2.contains(words[i])) {
            std::cout << "FAILED - word not found after deserialize: " << words[i] << "\n";
            sbfSerializePassed = false;
            break;
        }
    }
    if (sbfSerializePassed) std::cout << "PASSED - all words found after deserialize\n\n";

    std::cout << "=== Scalable Filter Stats ===\n";
    std::cout << "Total filters created: " << sbf.currentFilterCount() << "\n";
    std::cout << "Current FPR:           " << sbf.currentFPR() * 100 << "%\n";
    std::cout << "Insert time:           " << sbfInsertTime << " ms\n";
    std::cout << "Query time:            " << sbfQueryTime << " ms\n";
    std::cout << "Words inserted:        150,000\n";
    std::cout << "Initial capacity:      50,000\n";

    return 0;
}
