#include "conflicts.h"

#include <cstdint>

#include "assembly.h"
#include "constants.h"
#include "utils.h"

namespace sudoku {

void AccessTimingSingleMemoryAccess(uint64_t addr, uint64_t** histogram) {
  // measure
  for (size_t i = 0; i < SUDOKU_CONFLICT_NUM_ITERATION; ++i) {
    clflushopt(reinterpret_cast<void*>(addr));
    mfence();
    histogram[i][0] = rdtscp();
    *(volatile char*)addr;
    lfence();
    histogram[i][1] = rdtscp();
  }
  for (size_t i = 0; i < SUDOKU_CONFLICT_NUM_ITERATION; ++i) {
    histogram[i][1] -= histogram[i][0];
  }
}

uint64_t MedianAccessTimingSingleMemoryAccess(uint64_t addr) {
  uint64_t** histogram = AllocateHistogram(SUDOKU_CONFLICT_NUM_ITERATION, 2);
  AccessTimingSingleMemoryAccess(addr, histogram);
  uint64_t med = GetMedian(histogram, SUDOKU_CONFLICT_NUM_ITERATION, 2);
  FreeHistogram(histogram, SUDOKU_CONFLICT_NUM_ITERATION);
  return med;
}

uint64_t AverageAccessTimingSingleMemoryAccess(uint64_t addr) {
  uint64_t** histogram = AllocateHistogram(SUDOKU_CONFLICT_NUM_ITERATION, 2);
  AccessTimingSingleMemoryAccess(addr, histogram);
  uint64_t avg = GetAverage(histogram, SUDOKU_CONFLICT_NUM_ITERATION, 2);
  FreeHistogram(histogram, SUDOKU_CONFLICT_NUM_ITERATION);
  return avg;
}

void StatAccessTimingSingleMemoryAccess(uint64_t addr, uint64_t* results) {
  uint64_t** histogram = AllocateHistogram(SUDOKU_CONFLICT_NUM_ITERATION, 2);
  AccessTimingSingleMemoryAccess(addr, histogram);
  GetStatistics(histogram, SUDOKU_CONFLICT_NUM_ITERATION, 1, results);
  FreeHistogram(histogram, SUDOKU_CONFLICT_NUM_ITERATION);
}

void AccessTimingPairedMemoryAccess(uint64_t faddr, uint64_t saddr,
                                    uint64_t** histogram) {
  for (size_t i = 0; i < SUDOKU_CONFLICT_NUM_ITERATION; ++i) {
    clflushopt(reinterpret_cast<void*>(faddr));
    clflushopt(reinterpret_cast<void*>(saddr));
    mfence();
    histogram[i][0] = rdtscp();
    *(volatile char*)faddr;
    *(volatile char*)saddr;
    lfence();
    histogram[i][1] = rdtscp();
  }
  for (size_t i = 0; i < SUDOKU_CONFLICT_NUM_ITERATION; ++i) {
    histogram[i][1] -= histogram[i][0];
  }
}

uint64_t MedianAccessTimingPairedMemoryAccess(uint64_t faddr, uint64_t saddr) {
  uint64_t** histogram = AllocateHistogram(SUDOKU_CONFLICT_NUM_ITERATION, 2);
  AccessTimingPairedMemoryAccess(faddr, saddr, histogram);
  uint64_t med = GetMedian(histogram, SUDOKU_CONFLICT_NUM_ITERATION, 1);
  FreeHistogram(histogram, SUDOKU_CONFLICT_NUM_ITERATION);
  return med;
}

uint64_t AverageAccessTimingPairedMemoryAccess(uint64_t faddr, uint64_t saddr) {
  uint64_t** histogram = AllocateHistogram(SUDOKU_CONFLICT_NUM_ITERATION, 2);
  AccessTimingPairedMemoryAccess(faddr, saddr, histogram);
  uint64_t avg = GetAverage(histogram, SUDOKU_CONFLICT_NUM_ITERATION, 1);
  FreeHistogram(histogram, SUDOKU_CONFLICT_NUM_ITERATION);
  return avg;
}

void StatAccessTimingPairedMemoryAccess(uint64_t faddr, uint64_t saddr,
                                        uint64_t* results) {
  uint64_t** histogram = AllocateHistogram(SUDOKU_CONFLICT_NUM_ITERATION, 2);
  AccessTimingPairedMemoryAccess(faddr, saddr, histogram);
  GetStatistics(histogram, SUDOKU_CONFLICT_NUM_ITERATION, 1, results);
  FreeHistogram(histogram, SUDOKU_CONFLICT_NUM_ITERATION);
}

}  // namespace sudoku