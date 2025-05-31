#include "consecutive_accesses.h"

#include <stddef.h>

#include <algorithm>
#include <cstdint>
#include <string>

#include "assembly.h"
#include "constants.h"
#include "utils.h"

namespace sudoku {

void ReadReadLatency(uint64_t* faddrs, uint64_t* saddrs, size_t length,
                     uint64_t** histogram) {
  for (size_t i = 0; i < SUDOKU_CONSECUTIVE_NUM_ITERATION; ++i) {
    // clflushopt
    for (size_t j = 0; j < length; ++j) {
      clflushopt(reinterpret_cast<void*>(faddrs[j]));
      clflushopt(reinterpret_cast<void*>(saddrs[j]));
    }
    mfence();
    histogram[i][0] = rdtscp();
    // let MCs schedule the requests in this loop
    for (size_t j = 0; j < length; ++j) {
      *(volatile char*)faddrs[j];
      *(volatile char*)saddrs[j];
    }
    mfence();
    histogram[i][1] = rdtscp();
  }
  for (size_t i = 0; i < SUDOKU_CONSECUTIVE_NUM_ITERATION; ++i) {
    histogram[i][1] -= histogram[i][0];
  }
}

uint64_t MedianReadReadLatency(uint64_t* faddrs, uint64_t* saddrs,
                               size_t length) {
  uint64_t** histogram = AllocateHistogram(SUDOKU_CONSECUTIVE_NUM_ITERATION, 2);
  ReadReadLatency(faddrs, saddrs, length, histogram);
  uint64_t med = GetMedian(histogram, SUDOKU_CONSECUTIVE_NUM_ITERATION, 1);
  FreeHistogram(histogram, SUDOKU_CONSECUTIVE_NUM_ITERATION);
  return med;
}

uint64_t AverageReadReadLatency(uint64_t* faddrs, uint64_t* saddrs,
                                size_t length) {
  uint64_t** histogram = AllocateHistogram(SUDOKU_CONSECUTIVE_NUM_ITERATION, 2);
  ReadReadLatency(faddrs, saddrs, length, histogram);
  uint64_t avg = GetAverage(histogram, SUDOKU_CONSECUTIVE_NUM_ITERATION, 1);
  FreeHistogram(histogram, SUDOKU_CONSECUTIVE_NUM_ITERATION);
  return avg;
}

void StatReadReadLatency(uint64_t* faddrs, uint64_t* saddrs, size_t length,
                         uint64_t* results) {
  uint64_t** histogram = AllocateHistogram(SUDOKU_CONSECUTIVE_NUM_ITERATION, 2);
  ReadReadLatency(faddrs, saddrs, length, histogram);
  GetStatistics(histogram, SUDOKU_CONSECUTIVE_NUM_ITERATION, 1, results);
  FreeHistogram(histogram, SUDOKU_CONSECUTIVE_NUM_ITERATION);
}

}  // namespace sudoku