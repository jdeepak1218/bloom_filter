#include "bloom_filter.h"
#include "counting_bloom_filter.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include<fstream>
#include <mutex>
#include <shared_mutex>
#include<stdexcept>
CountingBloomFilter::CountingBloomFilter(size_t n,double p){
  if(n == 0)throw std::invalid_argument("n must be non negative");
  if(p <= 0 || p >= 1) throw std::invalid_argument("p must lie in range 0 to 1");
  this->n = n;
  this->p = p;
  m = computeM(n,p);
  k = computeK(m,n);
  count_arr.resize(m,0);
}
size_t CountingBloomFilter::computeM(size_t n,double p){
  return static_cast<size_t>(std::ceil(-static_cast<double>(n) * std::log(p) / (std::log(2) * std::log(2))));
}
size_t CountingBloomFilter::computeK(size_t m, size_t n){
  return static_cast<size_t>(std::round((static_cast<double>(m) / static_cast<double>(n)) * std::log(2)));
}
std::vector<uint64_t> CountingBloomFilter::getPositions(const std::string &key) const{
  uint64_t hashes[2];
  MurmurHash3_x64_128(key.c_str(),key.size(),42,hashes);
  uint64_t h1 = hashes[0];
  uint64_t h2 = hashes[1];
  std::vector<uint64_t>positions;
  for(size_t i = 0 ; i < k ; i++)
  {
    uint64_t pos = (h1 + i * h2) % m;
    positions.push_back(pos);
  }
  return positions;
}
void CountingBloomFilter::add(const std::string &key){
  std::vector<uint64_t> positions = getPositions(key);
  std::unique_lock<std::shared_mutex>lock(mutex_);
  for(auto pos : positions){
    if(count_arr[pos] < 255) count_arr[pos]++;
  }
}
void CountingBloomFilter::remove(const std::string &key)
{
  std::vector<uint64_t> positions = getPositions(key);
  std::unique_lock<std::shared_mutex>lock(mutex_);
  for(auto pos : positions){
    if(count_arr[pos] > 0) count_arr[pos]--;
  }
}
bool CountingBloomFilter::contains(const std::string &key) const{
  std::vector<uint64_t> positions = getPositions(key);
  std::shared_lock<std::shared_mutex>lock(mutex_);
  for(auto pos : positions){
    if(count_arr[pos] == 0){
      return false;
    }
  }
  return true;
}
double CountingBloomFilter::currentFPR() const {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  size_t numOfSetBits = 0;
  for(auto count : count_arr){
    if(count > 0) numOfSetBits++;
  }
  double fpr = std::pow(static_cast<double>(numOfSetBits) / static_cast<double>(m), k);
  return fpr;
}
size_t CountingBloomFilter::getM() const { return m; }
size_t CountingBloomFilter::getK() const { return k; }
void CountingBloomFilter::serialize(const std::string &filename) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    std::ofstream file(filename, std::ios::binary);
    if(!file) throw std::runtime_error("Could not open the file");
    file.write(reinterpret_cast<const char*>(&m), sizeof(m));
    file.write(reinterpret_cast<const char*>(&n), sizeof(n));
    file.write(reinterpret_cast<const char*>(&k), sizeof(k));
    file.write(reinterpret_cast<const char*>(count_arr.data()), m * sizeof(uint8_t));
}
void CountingBloomFilter::deserialize(const std::string &filename) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    std::ifstream file(filename, std::ios::binary);
    if(!file) throw std::runtime_error("Could not open the file");
    file.read(reinterpret_cast<char*>(&m), sizeof(m));
    file.read(reinterpret_cast<char*>(&n), sizeof(n));
    file.read(reinterpret_cast<char*>(&k), sizeof(k));
    count_arr.resize(m, 0);
    file.read(reinterpret_cast<char*>(count_arr.data()), m * sizeof(uint8_t));
}
