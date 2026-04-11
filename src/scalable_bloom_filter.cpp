#include"scalable_bloom_filter.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include<fstream>
#include <mutex>
#include <shared_mutex>
#include<stdexcept>

ScalableBloomFilter::ScalableBloomFilter(size_t n,double p){
  if(n == 0)throw std::invalid_argument("n must be positive");
  if(p <=0 || p >= 1)throw std::invalid_argument("p must lie b/w 0 and 1");
  initialN = n;
  initialP = p;
  growthFactor = 2;
  tightRatio = 0.85;
  insertionCount = 0;
  currentCapacity = n;
  filters.emplace_back(n,p);
}
void ScalableBloomFilter::addNewFilter(){
  size_t newCapacity = currentCapacity * growthFactor;
  double newP = initialP * std::pow(tightRatio,filters.size());
  filters.emplace_back(newCapacity,newP);
  currentCapacity = newCapacity;
}
void ScalableBloomFilter::add(const std::string &key){
  std::unique_lock<std::shared_mutex>lock(mutex_);
  if(insertionCount >= currentCapacity){
    addNewFilter();
  }
  filters.back().add(key);
  insertionCount++;
}
bool ScalableBloomFilter::contains(const std::string &key)const{
  std::shared_lock<std::shared_mutex>lock(mutex_);
  for(auto &filter : filters){
    if(filter.contains(key))return true;
  }
  return false;
}
double ScalableBloomFilter::currentFPR()const{
  std::shared_lock<std::shared_mutex>lock(mutex_);
  double fpr = 1.0;
  for(auto &filter: filters){
    fpr *= (1.0 - filter.currentFPR());
  }
  return 1.0 - fpr;
}
size_t ScalableBloomFilter::currentFilterCount()const{
  std::shared_lock<std::shared_mutex>lock(mutex_);
  return filters.size();
}
void ScalableBloomFilter::serialize(const std::string &filename){
  std::shared_lock<std::shared_mutex>lock(mutex_);
  std::ofstream file(filename,std::ios::binary);
  if(!file)throw std::runtime_error("Can't open file");
  file.write(reinterpret_cast<const char *>(&initialN),sizeof(initialN));
  file.write(reinterpret_cast<const char *>(&initialP),sizeof(initialP));
  file.write(reinterpret_cast<const char *>(&growthFactor),sizeof(growthFactor));
  file.write(reinterpret_cast<const char *>(&tightRatio),sizeof(tightRatio));
  file.write(reinterpret_cast<const char *>(&insertionCount),sizeof(insertionCount));
  file.write(reinterpret_cast<const char *>(&currentCapacity),sizeof(currentCapacity));
  size_t filterSize = filters.size();
  file.write(reinterpret_cast<const char *>(&filterSize),sizeof(filterSize));
  for(size_t i = 0; i < filters.size(); i++) {
      filters[i].serialize(filename + "_" + std::to_string(i) + ".bin");
  }
}

void ScalableBloomFilter::deserialize(const std::string &filename){
  std::unique_lock<std::shared_mutex>lock(mutex_);
  std::ifstream file(filename,std::ios::binary);
  if(!file)throw std::runtime_error("Can't open file");
  file.read(reinterpret_cast<char *>(&initialN),sizeof(initialN));
  file.read(reinterpret_cast<char *>(&initialP),sizeof(initialP));
  file.read(reinterpret_cast<char *>(&growthFactor),sizeof(growthFactor));
  file.read(reinterpret_cast<char *>(&tightRatio),sizeof(tightRatio));
  file.read(reinterpret_cast<char *>(&insertionCount),sizeof(insertionCount));
  file.read(reinterpret_cast<char *>(&currentCapacity),sizeof(currentCapacity));
  size_t filterSize = 0;
  file.read(reinterpret_cast<char *>(&filterSize),sizeof(filterSize));
  filters.clear();
  for(size_t i = 0; i < filterSize; i++) {
      filters.emplace_back(initialN,initialP);
      filters[i].deserialize(filename + "_" + std::to_string(i) + ".bin");
  }
}
