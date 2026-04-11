#include "bloom_filter.h"
#include "counting_bloom_filter.h"
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
    while (wordFile >> word && words.size() < 60000) {
        words.push_back(word);
    }
    wordFile.close();

    if (words.size() < 60000) {
        std::cerr << "Not enough words in file. Need at least 60000, got " << words.size() << "\n";
        return 1;
    }

    // =========================================
    //         BLOOM FILTER TESTS
    // =========================================
    std::cout << "\n========================================\n";
    std::cout << "         BLOOM FILTER TESTS\n";
    std::cout << "========================================\n\n";

    BloomFilter bf(50000, 0.09);

    // ─── Insertion ────────────────────────────────────────────────────────
    auto insertStart = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < 50000; i++) {
        bf.add(words[i]);
    }
    auto insertEnd = std::chrono::high_resolution_clock::now();
    double insertTime = std::chrono::duration<double, std::milli>(insertEnd - insertStart).count();

    std::cout << "=== Insertion ===\n";
    std::cout << "Inserted 50,000 words in " << insertTime << " ms\n\n";

    // ─── Zero false negatives test ────────────────────────────────────────
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

    // ─── FPR test ─────────────────────────────────────────────────────────
    std::cout << "=== False Positive Rate Test ===\n";
    auto queryStart = std::chrono::high_resolution_clock::now();
    size_t falsePositives = 0;
    for (size_t i = 50000; i < 60000; i++) {
        if (bf.contains(words[i])) {
            falsePositives++;
        }
    }
    auto queryEnd = std::chrono::high_resolution_clock::now();
    double queryTime = std::chrono::duration<double, std::milli>(queryEnd - queryStart).count();

    double measuredFPR = static_cast<double>(falsePositives) / 10000.0;
    std::cout << "False positives: " << falsePositives << " / 10000\n";
    std::cout << "Measured FPR:    " << measuredFPR * 100 << "%\n";
    std::cout << "Expected FPR:    9%\n";
    std::cout << "Queried 10,000 words in " << queryTime << " ms\n\n";

    // ─── Serialize / Deserialize test ─────────────────────────────────────
    std::cout << "=== Serialize / Deserialize Test ===\n";
    bf.serialize("bloom.bin");
    std::cout << "Filter saved to bloom.bin\n";

    BloomFilter bf2(50000, 0.09);
    bf2.deserialize("bloom.bin");
    std::cout << "Filter loaded from bloom.bin\n";

    bool serializePassed = true;
    for (size_t i = 0; i < 50000; i++) {
        if (!bf2.contains(words[i])) {
            std::cout << "FAILED - word not found after deserialize: " << words[i] << "\n";
            serializePassed = false;
            break;
        }
    }
    if (serializePassed) std::cout << "PASSED - all words found after deserialize\n\n";

    // ─── Stats ────────────────────────────────────────────────────────────
    std::cout << "=== Filter Stats ===\n";
    std::cout << "m (bit array size): " << bf.getM() << " bits ("
              << bf.getM() / 8 / 1024 << " KB)\n";
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

    // ─── Insertion ────────────────────────────────────────────────────────
    auto cbfInsertStart = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < 50000; i++) {
        cbf.add(words[i]);
    }
    auto cbfInsertEnd = std::chrono::high_resolution_clock::now();
    double cbfInsertTime = std::chrono::duration<double, std::milli>(cbfInsertEnd - cbfInsertStart).count();

    std::cout << "=== Insertion ===\n";
    std::cout << "Inserted 50,000 words in " << cbfInsertTime << " ms\n\n";

    // ─── Zero false negatives test ────────────────────────────────────────
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

    // ─── Deletion test (key feature!) ─────────────────────────────────────
    std::cout << "=== Deletion Test ===\n";
    std::vector<std::string> testWords = {
        words[0], words[1], words[2], words[3], words[4]
    };

    bool allFoundBefore = true;
    for (auto& w : testWords) {
        if (!cbf.contains(w)) {
            allFoundBefore = false;
            std::cout << "FAILED - word not found before deletion: " << w << "\n";
        }
    }
    if (allFoundBefore) std::cout << "Before deletion - all 5 words found\n";

    for (auto& w : testWords) {
        cbf.remove(w);
    }

    bool allRemovedAfter = true;
    for (auto& w : testWords) {
        if (cbf.contains(w)) {
            allRemovedAfter = false;
            std::cout << "FAILED - word still found after deletion: " << w << "\n";
        }
    }
    if (allRemovedAfter) std::cout << "After deletion  - all 5 words correctly removed\n";

    bool othersIntact = true;
    for (size_t i = 5; i < 100; i++) {
        if (!cbf.contains(words[i])) {
            othersIntact = false;
            std::cout << "FAILED - non-deleted word missing: " << words[i] << "\n";
            break;
        }
    }
    if (othersIntact) std::cout << "Other words still intact after deletion\n\n";

    // ─── FPR test ─────────────────────────────────────────────────────────
    std::cout << "=== False Positive Rate Test ===\n";
    auto cbfQueryStart = std::chrono::high_resolution_clock::now();
    size_t cbfFalsePositives = 0;
    for (size_t i = 50000; i < 60000; i++) {
        if (cbf.contains(words[i])) {
            cbfFalsePositives++;
        }
    }
    auto cbfQueryEnd = std::chrono::high_resolution_clock::now();
    double cbfQueryTime = std::chrono::duration<double, std::milli>(cbfQueryEnd - cbfQueryStart).count();

    double cbfMeasuredFPR = static_cast<double>(cbfFalsePositives) / 10000.0;
    std::cout << "False positives: " << cbfFalsePositives << " / 10000\n";
    std::cout << "Measured FPR:    " << cbfMeasuredFPR * 100 << "%\n";
    std::cout << "Expected FPR:    9%\n";
    std::cout << "Queried 10,000 words in " << cbfQueryTime << " ms\n\n";

    // ─── Serialize / Deserialize test ─────────────────────────────────────
    std::cout << "=== Serialize / Deserialize Test ===\n";
    cbf.serialize("counting_bloom.bin");
    std::cout << "Filter saved to counting_bloom.bin\n";

    CountingBloomFilter cbf2(50000, 0.09);
    cbf2.deserialize("counting_bloom.bin");
    std::cout << "Filter loaded from counting_bloom.bin\n";

    bool cbfSerializePassed = true;
    for (size_t i = 5; i < 50000; i++) {
        if (!cbf2.contains(words[i])) {
            std::cout << "FAILED - word not found after deserialize: " << words[i] << "\n";
            cbfSerializePassed = false;
            break;
        }
    }
    if (cbfSerializePassed) std::cout << "PASSED - all words found after deserialize\n\n";

    // ─── Stats ────────────────────────────────────────────────────────────
    std::cout << "=== Counting Filter Stats ===\n";
    std::cout << "m (array size):     " << cbf.getM() << " counters\n";
    std::cout << "k (hash functions): " << cbf.getK() << "\n";
    std::cout << "Current FPR:        " << cbf.currentFPR() * 100 << "%\n";
    std::cout << "Insert time:        " << cbfInsertTime << " ms\n";
    std::cout << "Query time:         " << cbfQueryTime << " ms\n";
    std::cout << "Memory usage:       " << cbf.getM() / 1024 << " KB\n";

    return 0;
}
