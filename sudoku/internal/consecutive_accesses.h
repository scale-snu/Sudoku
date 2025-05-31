#ifndef SUDOKU_INTERNAL_CONSECUTIVE_ACCESSES_H
#define SUDOKU_INTERNAL_CONSECUTIVE_ACCESSES_H

#include <stddef.h>

#include <cstdint>
#include <string>

namespace sudoku {

void ReadReadLatency(uint64_t* faddrs, uint64_t* saddrs, size_t length,
                     uint64_t** histogram);
uint64_t MedianReadReadLatency(uint64_t* faddrs, uint64_t* saddrs,
                               size_t length);
uint64_t AverageReadReadLatency(uint64_t* faddrs, uint64_t* saddrs,
                                size_t length);
void StatReadReadLatency(uint64_t* faddrs, uint64_t* saddrs, size_t length,
                         uint64_t* results);

}  // namespace sudoku

#endif  // SUDOKU_INTERNAL_CONSECUTIVE_ACCESSES_H