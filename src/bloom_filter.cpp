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
std::vector<uint64_t> BloomFilter::getPositions(const std::string &key) const{
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
void BloomFilter::add(const std::string &key){
  std::vector<uint64_t> positions = getPositions(key);
  for(auto pos : positions){
    bit_arr[pos] = true;
  }
}
bool BloomFilter::contains(const std::string &key) const{
  std::vector<uint64_t> positions = getPositions(key);
  for(auto pos : positions){
    if(!bit_arr[pos]){
      return false;
    }
  }
  return true;
}
double BloomFilter::currentFPR()const{
  size_t numOfSetBits = 0;
  for(auto bit : bit_arr){
    if(bit) numOfSetBits++;
  }
  double fpr = std::pow(static_cast<double>(numOfSetBits) / static_cast<double>(m), k);
  return fpr;
}
void BloomFilter::serialize(const std::string &filename) {
  std::ofstream file(filename,std::ios::binary);
  if(!file)throw std::runtime_error("Could not open the file");
  file.write(reinterpret_cast<const char *>(&m),sizeof(m));
  file.write(reinterpret_cast<const char *>(&n),sizeof(n));
  file.write(reinterpret_cast<const char *>(&k),sizeof(k));
  size_t numOfBytes = (m + 7) / 8;
  for(size_t i = 0 ; i < numOfBytes ; i++)
  {
    uint8_t byte = 0;
    for(size_t j = 0 ; j <= 7 ; j++)
    {
      size_t index = i * 8 + j;
      if(index < m && bit_arr[index]){
        byte |= (1 << j);
      }
    }
    file.write(reinterpret_cast<const char *>(&byte),sizeof(byte));
  }
}
