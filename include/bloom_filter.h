#pragma once
#include "murmur3.h"
#include <cstddef>
#include <vector>
#include<shared_mutex>
#include <string>
#include <cstdint>
class BloomFilter{
  private:
  double p;
  std::vector<bool> bit_arr;
  mutable std::shared_mutex mutex_;
  size_t m;
  size_t k;
  size_t n;
  size_t computeM(size_t n,double p);
  size_t computeK(size_t m,size_t n);
  std::vector<uint64_t> getPositions(const std::string &key) const;

  public:
  BloomFilter(size_t n,double p);
  void add(const std::string &key);
  BloomFilter(BloomFilter&& other) noexcept;
  BloomFilter& operator=(BloomFilter&& other) noexcept;
  bool contains(const std::string &key)const;
  double currentFPR()const;
  size_t getM() const;
  size_t getK() const;
  void serialize(const std::string &filename);
  void deserialize(const std::string &filename);
};
