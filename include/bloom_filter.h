#pragma once
#include "murmur3.h"
#include <cstddef>
#include <vector>
#include <string>
#include <cstdint>
class BloomFilter{
  private:
  double p;
  std::vector<bool> bit_arr;
  size_t m;
  size_t k;
  size_t n;
  size_t computeM(size_t n,double p);
  size_t computeK(size_t m,size_t n);
  std::vector<uint64_t> getPositions(const std::string &key);

  public:
  BloomFilter(size_t n,double p);
  void add(const std::string &key);
  bool contains(const std::string &key);
  double currentFPR();
  void serialize(const std::string &filename);
  void deserialize(const std::string &filename);
};
