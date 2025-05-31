#include "refreshes.h"

#include <algorithm>
#include <cstdint>

#include "assembly.h"
#include "constants.h"
#include "utils.h"

namespace sudoku {

void FilterRefreshTiming(uint64_t** histogram, uint64_t num_cols,
                         uint64_t threshold, std::vector<uint64_t>& refreshes) {
  if (num_cols == 2) {
    for (size_t i = 0; i < SUDOKU_REFRESH_NUM_ITERATION; ++i) {
      if (histogram[i][1] > threshold) {
        refreshes.push_back(histogram[i][0] - histogram[0][0]);
      }
    }
  } else if (num_cols == 3) {
    for (size_t i = 0; i < SUDOKU_REFRESH_NUM_ITERATION; ++i) {
      if (histogram[i][2] > threshold) {
        refreshes.push_back(histogram[i][0] - histogram[0][0] +
                            histogram[i][1]);
      }
    }
  } else {
    PRINT_ERROR("Unsupported num_cols: ", num_cols);
  }
}

std::vector<uint64_t> ComputeRefreshIntervals(
    const std::vector<uint64_t>& refreshes) {
  std::vector<uint64_t> intervals;
  // Check size of refreshes
  if (refreshes.size() < 2) {
    // PRINT_ERROR("No refresh detected. Please change REFRESH_CYCLE.");
    return intervals;
  }
  // Compute intervals
  for (size_t i = 1; i < refreshes.size(); ++i) {
    uint64_t delta = refreshes[i] - refreshes[i - 1];
    intervals.push_back(delta);
  }
  return intervals;
}

void MeasureRefreshSingleAccess(uint64_t addr, uint64_t** histogram) {
  for (size_t i = 0; i < SUDOKU_REFRESH_NUM_ITERATION; ++i) {
    clflushopt(reinterpret_cast<void*>(addr));
    mfence();
    histogram[i][0] = rdtscp();
    *(volatile char*)addr;
    lfence();
    histogram[i][1] = rdtscp();
  }
  for (size_t i = 0; i < SUDOKU_REFRESH_NUM_ITERATION; ++i) {
    histogram[i][1] -= histogram[i][0];
  }
}

uint64_t MedianRefreshIntervalSingleAccess(uint64_t addr, uint64_t threshold) {
  uint64_t** histogram = AllocateHistogram(SUDOKU_REFRESH_NUM_ITERATION, 2);
  MeasureRefreshSingleAccess(addr, histogram);
  // Filter refresh timings and compute intervals
  std::vector<uint64_t> refreshes;
  FilterRefreshTiming(histogram, 2, threshold, refreshes);
  std::vector<uint64_t> intervals = ComputeRefreshIntervals(refreshes);
  FreeHistogram(histogram, SUDOKU_REFRESH_NUM_ITERATION);
  return GetMedian(intervals);
}

uint64_t AverageRefreshIntervalSingleAccess(uint64_t addr, uint64_t threshold) {
  uint64_t** histogram = AllocateHistogram(SUDOKU_REFRESH_NUM_ITERATION, 2);
  MeasureRefreshSingleAccess(addr, histogram);
  // Filter refresh timings and compute intervals
  std::vector<uint64_t> refreshes;
  FilterRefreshTiming(histogram, 2, threshold, refreshes);
  std::vector<uint64_t> intervals = ComputeRefreshIntervals(refreshes);
  FreeHistogram(histogram, SUDOKU_REFRESH_NUM_ITERATION);
  return GetAverage(intervals);
}

void StatRefreshIntervalSingleAccess(uint64_t addr, uint64_t threshold,
                                     uint64_t* results) {
  uint64_t** histogram = AllocateHistogram(SUDOKU_REFRESH_NUM_ITERATION, 2);
  MeasureRefreshSingleAccess(addr, histogram);
  // Filter refresh timings and compute intervals
  std::vector<uint64_t> refreshes;
  FilterRefreshTiming(histogram, 2, threshold, refreshes);
  std::vector<uint64_t> intervals = ComputeRefreshIntervals(refreshes);
  GetStatistics(intervals, results);
  FreeHistogram(histogram, SUDOKU_REFRESH_NUM_ITERATION);
}

void MeasureRefreshPairedAccessCoarse(uint64_t faddr, uint64_t saddr,
                                      uint64_t** histogram) {
  for (size_t i = 0; i < SUDOKU_REFRESH_NUM_ITERATION; ++i) {
    clflushopt(reinterpret_cast<void*>(faddr));
    clflushopt(reinterpret_cast<void*>(saddr));
    mfence();
    histogram[i][0] = rdtscp();
    *(volatile char*)faddr;
    *(volatile char*)saddr;
    lfence();
    histogram[i][1] = rdtscp();
  }
  for (size_t i = 0; i < SUDOKU_REFRESH_NUM_ITERATION; ++i) {
    histogram[i][1] -= histogram[i][0];
  }
}

uint64_t MedianRefreshIntervalPairedAccessCoarse(uint64_t faddr, uint64_t saddr,
                                                 uint64_t threshold) {
  uint64_t** histogram = AllocateHistogram(SUDOKU_REFRESH_NUM_ITERATION, 2);
  MeasureRefreshPairedAccessCoarse(faddr, saddr, histogram);
  // Filter refresh timings and compute intervals
  std::vector<uint64_t> refreshes;
  FilterRefreshTiming(histogram, 2, threshold, refreshes);
  std::vector<uint64_t> intervals = ComputeRefreshIntervals(refreshes);
  FreeHistogram(histogram, SUDOKU_REFRESH_NUM_ITERATION);
  return GetMedian(intervals);
}

uint64_t AverageRefreshIntervalPairedAccessCoarse(uint64_t faddr,
                                                  uint64_t saddr,
                                                  uint64_t threshold) {
  uint64_t** histogram = AllocateHistogram(SUDOKU_REFRESH_NUM_ITERATION, 2);
  MeasureRefreshPairedAccessCoarse(faddr, saddr, histogram);
  // Filter refresh timings and compute intervals
  std::vector<uint64_t> refreshes;
  FilterRefreshTiming(histogram, 2, threshold, refreshes);
  std::vector<uint64_t> intervals = ComputeRefreshIntervals(refreshes);
  FreeHistogram(histogram, SUDOKU_REFRESH_NUM_ITERATION);
  return GetAverage(intervals);
}

void StatRefreshIntervalPairedAccessCoarse(uint64_t faddr, uint64_t saddr,
                                           uint64_t threshold,
                                           uint64_t* results) {
  uint64_t** histogram = AllocateHistogram(SUDOKU_REFRESH_NUM_ITERATION, 2);
  MeasureRefreshPairedAccessCoarse(faddr, saddr, histogram);
  // Filter refresh timings and compute intervals
  std::vector<uint64_t> refreshes;
  FilterRefreshTiming(histogram, 2, threshold, refreshes);
  std::vector<uint64_t> intervals = ComputeRefreshIntervals(refreshes);
  GetStatistics(intervals, results);
  FreeHistogram(histogram, SUDOKU_REFRESH_NUM_ITERATION);
}

void MeasureRefreshPairedAccessFine(uint64_t faddr, uint64_t saddr,
                                    uint64_t** histogram) {
  for (size_t i = 0; i < SUDOKU_REFRESH_NUM_ITERATION; ++i) {
    clflushopt(reinterpret_cast<void*>(faddr));
    clflushopt(reinterpret_cast<void*>(saddr));
    mfence();
    histogram[i][0] = rdtscp();
    *(volatile char*)faddr;
    lfence();
    histogram[i][1] = rdtscp();
    *(volatile char*)saddr;
    lfence();
    histogram[i][2] = rdtscp();
  }
  for (size_t i = 0; i < SUDOKU_REFRESH_NUM_ITERATION; ++i) {
    histogram[i][2] -= histogram[i][1];
    histogram[i][1] -= histogram[i][0];
  }
}

uint64_t MedianFineRefreshIntervalPairedMemoryAccess(uint64_t faddr,
                                                     uint64_t saddr,
                                                     uint64_t threshold) {
  uint64_t** histogram = AllocateHistogram(SUDOKU_REFRESH_NUM_ITERATION, 3);
  MeasureRefreshPairedAccessFine(faddr, saddr, histogram);
  // Filter refresh timings and compute intervals
  std::vector<uint64_t> refreshes;
  FilterRefreshTiming(histogram, 3, threshold, refreshes);
  std::vector<uint64_t> intervals = ComputeRefreshIntervals(refreshes);
  FreeHistogram(histogram, SUDOKU_REFRESH_NUM_ITERATION);
  return GetMedian(intervals);
}

uint64_t AverageRefreshIntervalPairedAccessFine(uint64_t faddr, uint64_t saddr,
                                                uint64_t threshold) {
  uint64_t** histogram = AllocateHistogram(SUDOKU_REFRESH_NUM_ITERATION, 2);
  MeasureRefreshPairedAccessCoarse(faddr, saddr, histogram);
  // Filter refresh timings and compute intervals
  std::vector<uint64_t> refreshes;
  FilterRefreshTiming(histogram, 3, threshold, refreshes);
  std::vector<uint64_t> intervals = ComputeRefreshIntervals(refreshes);
  FreeHistogram(histogram, SUDOKU_REFRESH_NUM_ITERATION);
  return GetAverage(intervals);
}

void StatRefreshIntervalPairedAccessFine(uint64_t faddr, uint64_t saddr,
                                         uint64_t threshold,
                                         uint64_t* results) {
  uint64_t** histogram = AllocateHistogram(SUDOKU_REFRESH_NUM_ITERATION, 3);
  MeasureRefreshPairedAccessFine(faddr, saddr, histogram);
  // Filter refresh timings and compute intervals
  std::vector<uint64_t> refreshes;
  FilterRefreshTiming(histogram, 3, threshold, refreshes);
  std::vector<uint64_t> intervals = ComputeRefreshIntervals(refreshes);
  GetStatistics(intervals, results);
  FreeHistogram(histogram, SUDOKU_REFRESH_NUM_ITERATION);
}

}  // namespace sudoku
