#include"bloom_filter.h"
#include <cmath>
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
