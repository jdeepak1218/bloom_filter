#pragma once
#include "bloom_filter.h"
#include <vector>
#include <string>
#include <cstddef>
#include <shared_mutex>
class ScalableBloomFilter{
private:
std::vector<BloomFilter>filters;
size_t initialN;
double initialP;
double growthFactor;
double tightRatio;
size_t insertionCount;
size_t currentCapacity;
mutable std::shared_mutex mutex_;
void addNewFilter();
public:
ScalableBloomFilter(size_t n ,double p);
void add(const std::string &key);
bool contains(const std::string &key)const;
double currentFPR()const;
size_t currentFilterCount()const;
void serialize(const std::string &filename);
void deserialize(const std::string &filename);
};
