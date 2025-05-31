#ifndef SUDOKU_INTERNAL_POOL_H
#define SUDOKU_INTERNAL_POOL_H

#include <sys/mman.h>

#include <cstdint>
#include <iostream>
#include <random>

constexpr uint64_t MAP_HUGE_1GB =
    (30ULL << MAP_HUGE_SHIFT);  // we only use this
constexpr uint64_t MAP_HUGE_2MB = (21ULL << MAP_HUGE_SHIFT);

/// @brief Configuration for memory pool
struct MemoryPoolConfig {
  uint64_t page_size;
  uint64_t num_pages;
  uint64_t granularity;
  bool huge;
  uint64_t pool_size;

  MemoryPoolConfig()
      : page_size(4096),
        num_pages(4),
        granularity(64),
        huge(false),
        pool_size(page_size * num_pages) {}

  MemoryPoolConfig(uint64_t pa, uint64_t npages, uint64_t g, bool h)
      : page_size(pa),
        num_pages(npages),
        granularity(g),
        huge(h),
        pool_size(page_size * num_pages) {}
};

/// @brief Memory pool managing multiple OS pages
struct MemoryPool {
  char** pages = nullptr;
  MemoryPoolConfig* config = nullptr;

  std::mt19937 gen;
  std::uniform_int_distribution<uint64_t> page_dist;
  std::uniform_int_distribution<uint64_t> page_offset;

  MemoryPool() : config(new MemoryPoolConfig()) {}

  MemoryPool(uint64_t p, uint64_t n, uint64_t g, bool h)
      : config(new MemoryPoolConfig(p, n, g, h)) {}

  MemoryPool(MemoryPoolConfig* cfg) : config(cfg) {}
};

bool InitMemoryPool(MemoryPool* pool);
bool FreeMemoryPool(MemoryPool* pool);
bool UpdateMemoryPool(MemoryPool* pool, MemoryPoolConfig* cfg);

uint64_t VirtToPhys(uint64_t vaddr);
uint64_t PhysToVirt(MemoryPool* pool, uint64_t paddr);

#endif  // SUDOKU_INTERNAL_POOL_H