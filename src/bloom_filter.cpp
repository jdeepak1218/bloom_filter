#include"bloom_filter.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include<fstream>
#include<stdexcept>
BloomFilter::BloomFilter(size_t n,double p){
  this->n = n;
  this->p = p;
  m = computeM(n,p);
  k = computeK(m,n);
  bit_arr.resize(m,false);
}
size_t BloomFilter::computeM(size_t n,double p){
  return static_cast<size_t>(std::ceil(-static_cast<double>(n) * std::log(p) / (std::log(2) * std::log(2))));
}
size_t BloomFilter::computeK(size_t m, size_t n){
  return static_cast<size_t>(std::round((static_cast<double>(m) / static_cast<double>(n)) * std::log(2)));
}
std::vector<uint64_t> BloomFilter::getPositions(const std::string &key){
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
