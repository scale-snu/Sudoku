#ifndef SUDOKU_INTERNAL_CONFLICTS_H
#define SUDOKU_INTERNAL_CONFLICTS_H

#include <cstdint>

namespace sudoku {

// Single address access timing
void AccessTimingSingleMemoryAccess(uint64_t addr, uint64_t** histogram);
uint64_t MedianAccessTimingSingleMemoryAccess(uint64_t addr);
uint64_t AverageAccessTimingSingleMemoryAccess(uint64_t addr);
void StatAccessTimingSingleMemoryAccess(uint64_t addr, uint64_t* results);

// Paired address access timing
void AccessTimingPairedMemoryAccess(uint64_t faddr, uint64_t saddr,
                                    uint64_t** histogram);
uint64_t MedianAccessTimingPairedMemoryAccess(uint64_t faddr, uint64_t saddr);
uint64_t AverageAccessTimingPairedMemoryAccess(uint64_t faddr, uint64_t saddr);
void StatAccessTimingPairedMemoryAccess(uint64_t faddr, uint64_t saddr,
                                        uint64_t* results);

}  // namespace sudoku

#endif  // SUDOKU_INTERNAL_CONFLICTS_H