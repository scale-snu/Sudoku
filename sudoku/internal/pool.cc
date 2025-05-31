#include "pool.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cassert>
#include <iostream>
#include <random>

#include "utils.h"

bool InitMemoryPool(MemoryPool* pool) {
  uint32_t map_flag = MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE;
  if (pool->config->huge) {
    map_flag |= (MAP_HUGETLB | MAP_HUGE_1GB);
  }

  pool->config->pool_size = pool->config->num_pages * pool->config->page_size;
  pool->gen = std::mt19937(std::random_device{}());
  pool->page_dist =
      std::uniform_int_distribution<uint64_t>(0, pool->config->num_pages - 1);
  pool->page_offset =
      std::uniform_int_distribution<uint64_t>(0, pool->config->page_size - 1);

  pool->pages = new char*[pool->config->num_pages];

  for (uint64_t i = 0; i < pool->config->num_pages; ++i) {
    pool->pages[i] =
        static_cast<char*>(mmap(nullptr, pool->config->page_size,
                                PROT_READ | PROT_WRITE, map_flag, -1, 0));

    if (pool->pages[i] == MAP_FAILED) {
      PRINT_ERROR("[-] mmap failed\n");
      exit(EXIT_FAILURE);
    }
  }

  return true;
}

bool FreeMemoryPool(MemoryPool* pool) {
  if (!pool || !pool->pages || !pool->config) {
    return false;
  }

  for (uint64_t i = 0; i < pool->config->num_pages; ++i) {
    munmap(pool->pages[i], pool->config->page_size);
  }
  delete[] pool->pages;
  pool->pages = nullptr;

  return true;
}

bool UpdateMemoryPool(MemoryPool* pool, MemoryPoolConfig* cfg) {
  if (!FreeMemoryPool(pool)) {
    return false;
  }
  pool->config = cfg;
  return InitMemoryPool(pool);
}

uint64_t VirtToPhys(uint64_t vaddr) {
  const uint64_t page_size = sysconf(_SC_PAGESIZE);
  uint64_t paddr = 0;
  uint64_t offset = (vaddr / page_size) * sizeof(uint64_t);

  int fd = open("/proc/self/pagemap", O_RDONLY);
  if (fd < 0) {
    PRINT_ERROR("[-] Cannot open pagemap\n");
    exit(EXIT_FAILURE);
  }

  ssize_t read_size = pread(fd, &paddr, sizeof(paddr), offset);
  close(fd);

  if (read_size != sizeof(uint64_t)) {
    PRINT_ERROR("[-] Failed to read from pagemap\n");
    exit(EXIT_FAILURE);
  }

  assert(paddr & (1ULL << 63));  // page
  paddr &= 0x7fffffffffffffULL;

  return (paddr * page_size) | (vaddr % page_size);
}

uint64_t PhysToVirt(MemoryPool* pool, uint64_t paddr) {
  if (!pool || !pool->pages) {
    return 0;
  }
  // try to find the address from the memory pool (accessible)
  for (uint64_t i = 0; i < pool->config->num_pages; ++i) {
    uint64_t mem_start = reinterpret_cast<uint64_t>(pool->pages[i]);
    uint64_t mem_end =
        mem_start + pool->config->page_size - pool->config->granularity;

    uint64_t mem_start_p = VirtToPhys(mem_start);
    uint64_t mem_end_p = VirtToPhys(mem_end);

    if (mem_start_p <= paddr && paddr <= mem_end_p) {
      return mem_start + (paddr - mem_start_p);
    }
    if (mem_start_p >= mem_end_p && paddr >= mem_end_p &&
        paddr <= mem_start_p) {
      return mem_end + (paddr - mem_end_p);
    }
  }

  return 0;  // not found
}