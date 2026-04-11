#include "bloom_filter.h"
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <algorithm>
#include <cctype>
std::string normalize(std::string url) {
    std::transform(url.begin(), url.end(), url.begin(), ::tolower);
    if(url.substr(0, 8) == "https://") url = url.substr(8);
    else if(url.substr(0, 7) == "http://") url = url.substr(7);
    return url;
}
int main(int argc, char* argv[]) {
    if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <url>\n";
        std::cerr << "Example: url_checker \"http://suspicious-site.com\"\n";
        return 1;
    }
    std::string url = normalize(argv[1]);
    BloomFilter filter(70000, 0.01);
    std::ifstream binCheck("data/malicious_filter.bin");
    bool cacheExists = binCheck.good();
    binCheck.close();
    if(cacheExists) {
        auto loadStart = std::chrono::high_resolution_clock::now();
        filter.deserialize("data/malicious_filter.bin");
        auto loadEnd = std::chrono::high_resolution_clock::now();
        double loadTime = std::chrono::duration<double, std::milli>(loadEnd - loadStart).count();
        std::cout << "Filter loaded from cache in " << loadTime << " ms\n\n";
    } else {
        std::ifstream file("data/malicious_urls.txt");
        if(!file) {
            std::cerr << "Could not open data/malicious_urls.txt\n";
            return 1;
        }
        size_t count = 0;
        std::string line;
        auto loadStart = std::chrono::high_resolution_clock::now();
        while(std::getline(file, line)) {
            if(line.empty()) continue;
            if(line[0] == '#') continue;
            line = normalize(line);
            filter.add(line);
            count++;
        }
        auto loadEnd = std::chrono::high_resolution_clock::now();
        double loadTime = std::chrono::duration<double, std::milli>(loadEnd - loadStart).count();
        std::cout << "Loaded " << count << " malicious URLs in " << loadTime << " ms\n";
        filter.serialize("data/malicious_filter.bin");
        std::cout << "Filter cached to data/malicious_filter.bin\n\n";
    }
    auto checkStart = std::chrono::high_resolution_clock::now();
    bool isMalicious = filter.contains(url);
    auto checkEnd = std::chrono::high_resolution_clock::now();
    double checkTime = std::chrono::duration<double, std::nano>(checkEnd - checkStart).count();
    std::cout << "URL checked: " << url << "\n\n";
    if(isMalicious) {
        std::cout << "WARNING - possible malicious URL detected!\n";
        std::cout << "This URL may be in the malicious database.\n";
        std::cout << "Note: " << filter.currentFPR() * 100 << "% chance of false positive.\n";
    } else {
        std::cout << "SAFE - not found in malicious database.\n";
    }
    std::cout << "\n   Stats  \n";
    std::cout << "Check time:  " << checkTime << " ns\n";
    std::cout << "Filter FPR:  " << filter.currentFPR() * 100 << "%\n";
    std::cout << "Filter size: " << filter.getM() / 8 / 1024 << " KB\n";
    return 0;
}
