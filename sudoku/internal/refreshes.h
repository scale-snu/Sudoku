#ifndef SUDOKU_INTERNAL_REFRESHES_H
#define SUDOKU_INTERNAL_REFRESHES_H

#include <cstdint>
#include <vector>

namespace sudoku {

// Interval utilities
void FilterRefreshTiming(uint64_t** histogram, uint64_t num_cols,
                         uint64_t threshold, std::vector<uint64_t>& refreshes);
std::vector<uint64_t> ComputeRefreshIntervals(
    const std::vector<uint64_t>& refreshes);

// Auto-refresh on single memory accesses
void MeasureRefreshSingleAccess(uint64_t addr, uint64_t** histogram);
uint64_t MedianRefreshIntervalSingleAccess(uint64_t addr, uint64_t threshold);
uint64_t AverageRefreshIntervalSingleAccess(uint64_t addr, uint64_t threshold);
void StatRefreshIntervalSingleAccess(uint64_t addr, uint64_t threshold,
                                     uint64_t* results);

// Auto-refresh on paired memory accesses (coarse-grained measurements)
void MeasureRefreshPairedAccessCoarse(uint64_t faddr, uint64_t saddr,
                                      uint64_t** histogram);
uint64_t MedianRefreshIntervalPairedAccessCoarse(uint64_t faddr, uint64_t saddr,
                                                 uint64_t threshold);
uint64_t AverageRefreshIntervalPairedAccessCoarse(uint64_t faddr,
                                                  uint64_t saddr,
                                                  uint64_t threshold);
void StatRefreshIntervalPairedAccessCoarse(uint64_t faddr, uint64_t saddr,
                                           uint64_t threshold,
                                           uint64_t* results);

// Auto-refresh on paired memory accesses (fine-grained measurements)
void MeasureRefreshPairedAccessFine(uint64_t faddr, uint64_t saddr,
                                    uint64_t** histogram);
uint64_t MedianRefreshIntervalPairedAccessFine(uint64_t faddr, uint64_t saddr,
                                               uint64_t threshold);
uint64_t AverageRefreshIntervalPairedAccessFine(uint64_t faddr, uint64_t saddr,
                                                uint64_t threshold);
void StatRefreshIntervalPairedAccessFine(uint64_t faddr, uint64_t saddr,
                                         uint64_t threshold, uint64_t* results);

}  // namespace sudoku

#endif  // SUDOKU_INTERNAL_REFRESHES_H