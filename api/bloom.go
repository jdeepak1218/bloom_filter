package main

import (
	"encoding/binary"
	"math"
	"os"
	"sync"

	"github.com/spaolacci/murmur3"
)

type BloomFilter struct {
	bitArr []bool
	m      uint64
	k      uint64
	n      uint64
	p      float64
	count  uint64
	mu     sync.RWMutex
}

func NewBloomFilter(n uint64, p float64) *BloomFilter {
	m := math.Ceil(-(float64(n) * math.Log(p)) / (math.Log(2) * math.Log(2)))
	k := math.Round((m / float64(n)) * math.Log(2))
	return &BloomFilter{
		bitArr: make([]bool, uint64(m)),
		m:      uint64(m),
		k:      uint64(k),
		n:      n,
		p:      p,
		count:  0,
	}
}
func (bf *BloomFilter) getPositions(key string) []uint64 {
	h1 := murmur3.Sum64WithSeed([]byte(key), 0)
	h2 := murmur3.Sum64WithSeed([]byte(key), 1)
	positions := make([]uint64, bf.k)
	for i := uint64(0); i < bf.k; i++ {
		positions[i] = (h1 + i*h2) % bf.m
	}
	return positions
}
func (bf *BloomFilter) Add(key string) {
	positions := bf.getPositions(key)
	bf.mu.Lock()
	defer bf.mu.Unlock()
	for _, idx := range positions {
		bf.bitArr[idx] = true
	}
	bf.count++
}
func (bf *BloomFilter) Contains(key string) bool {
	positions := bf.getPositions(key)
	bf.mu.RLock()
	defer bf.mu.RUnlock()
	for _, idx := range positions {
		if !bf.bitArr[idx] {
			return false
		}
	}
	return true
}
func (bf *BloomFilter) CurrentFPR() float64 {
	bf.mu.RLock()
	defer bf.mu.RUnlock()
	var bitsSet uint64 = 0
	for i := uint64(0); i < bf.m; i++ {
		if bf.bitArr[i] {
			bitsSet++
		}
	}
	ratio := float64(bitsSet) / float64(bf.m)
	fpr := math.Pow(ratio, float64(bf.k))
	return fpr
}
func (bf *BloomFilter) Serialize(filename string) error {
	bf.mu.RLock()
	defer bf.mu.RUnlock()
	file, err := os.Create(filename)
	if err != nil {
		return err
	}
	defer file.Close()
	if err := binary.Write(file, binary.LittleEndian, bf.m); err != nil {
		return err
	}
	if err := binary.Write(file, binary.LittleEndian, bf.n); err != nil {
		return err
	}
	if err := binary.Write(file, binary.LittleEndian, bf.k); err != nil {
		return err
	}
	if err := binary.Write(file, binary.LittleEndian, bf.p); err != nil {
		return err
	}
	numBytes := (bf.m + 7) / 8
	for i := uint64(0); i < numBytes; i++ {
		var byteVal uint8 = 0
		for j := uint64(0); j < 8; j++ {
			idx := i*8 + j
			if idx < bf.m && bf.bitArr[idx] {
				byteVal |= (1 << j)
			}
		}
		binary.Write(file, binary.LittleEndian, byteVal)
	}
	return nil
}
func Deserialize(filename string) (*BloomFilter, error) {
	file, err := os.Open(filename)
	if err != nil {
		return nil, err
	}
	defer file.Close()
	bf := &BloomFilter{}
	binary.Read(file, binary.LittleEndian, &bf.m)
	binary.Read(file, binary.LittleEndian, &bf.n)
	binary.Read(file, binary.LittleEndian, &bf.k)
	binary.Read(file, binary.LittleEndian, &bf.p)
	bf.bitArr = make([]bool, bf.m)
	numBytes := (bf.m + 7) / 8
	for i := uint64(0); i < numBytes; i++ {
		var byteVal uint8
		binary.Read(file, binary.LittleEndian, &byteVal)
		for j := uint64(0); j < 8; j++ {
			index := i*8 + j
			if index < bf.m && (byteVal&(1<<j)) != 0 {
				bf.bitArr[index] = true
			}
		}
	}

	return bf, nil
}
