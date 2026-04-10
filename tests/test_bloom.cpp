#include "bloom_filter.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>

int main() {
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
    BloomFilter bf(50000, 0.09);
    auto insertStart = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < 50000; i++) {
        bf.add(words[i]);
    }
    auto insertEnd = std::chrono::high_resolution_clock::now();
    double insertTime = std::chrono::duration<double, std::milli>(insertEnd - insertStart).count();
    std::cout << "=== Insertion ===\n";
    std::cout << "Inserted 50,000 words in " << insertTime << " ms\n\n";
    std::cout << "=== Zero False Negatives Test ===\n";
    bool passed = true;
    for (size_t i = 0; i < 50000; i++) {
        if (!bf.contains(words[i])) {
            std::cout << "FAILED — word not found: " << words[i] << "\n";
            passed = false;
            break;
        }
    }
    if (passed) std::cout << "PASSED — all inserted words found\n\n";
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
    std::cout << "=== Serialize / Deserialize Test ===\n";
    bf.serialize("bloom.bin");
    std::cout << "Filter saved to bloom.bin\n";
    BloomFilter bf2(50000, 0.09);
    bf2.deserialize("bloom.bin");
    std::cout << "Filter loaded from bloom.bin\n";
    bool serializePassed = true;
    for (size_t i = 0; i < 50000; i++) {
        if (!bf2.contains(words[i])) {
            std::cout << "FAILED — word not found after deserialize: " << words[i] << "\n";
            serializePassed = false;
            break;
        }
    }
    if (serializePassed) std::cout << "PASSED — all words found after deserialize\n\n";
    std::cout << "=== Filter Stats ===\n";
    std::cout << "m (bit array size): " << bf.getM() << " bits ("
              << bf.getM() / 8 / 1024 << " KB)\n";
    std::cout << "k (hash functions): " << bf.getK() << "\n";
    std::cout << "Current FPR:        " << bf.currentFPR() * 100 << "%\n";
    std::cout << "Insert time:        " << insertTime << " ms\n";
    std::cout << "Query time:         " << queryTime << " ms\n";

    return 0;
}
