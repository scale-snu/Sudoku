#ifndef SUDOKU_INTERNAL_ASSEMBLY_H
#define SUDOKU_INTERNAL_ASSEMBLY_H

#include <stdint.h>

// Flush cache line optimized
static inline __attribute__((always_inline)) void clflushopt(volatile void* p) {
  asm volatile("clflushopt (%0)\n" : : "r"(p) : "memory");
}

// Load fence
static inline __attribute__((always_inline)) void lfence() {
  asm volatile("lfence\n" : : : "memory");
}

// Store fence
static inline __attribute__((always_inline)) void sfence() {
  asm volatile("sfence\n" : : : "memory");
}

// Memory fence
static inline __attribute__((always_inline)) void mfence() {
  asm volatile("mfence\n" : : : "memory");
}

// Read time-stamp counter and processor id
// https://github.com/amdprefetch/amd-prefetch-attacks/blob/master/case-studies/kaslr-break/cacheutils.h
static inline __attribute__((always_inline)) uint64_t rdtscp() {
  uint32_t a, d;
#ifndef IS_RDPRU
  asm volatile("rdtscp\n" : "=a"(a), "=d"(d) : : "ecx");
#else
  asm volatile(".byte 0x0f, 0x01, 0xfd\n" : "=a"(a), "=d"(d) : "c"());
#endif
  return ((uint64_t)d << 32) | a;
}

#endif  // SUDOKU_INTERNAL_ASSEMBLY_H