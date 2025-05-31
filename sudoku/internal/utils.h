#ifndef SUDOKU_INTERNAL_UTILS_H
#define SUDOKU_INTERNAL_UTILS_H

#include <spdlog/spdlog.h>

#include <cstdint>
#include <unordered_set>
#include <utility>
#include <vector>

namespace sudoku {

constexpr const char* color_red = "\033[31m";
constexpr const char* color_yellow = "\033[33m";
constexpr const char* color_green = "\033[32m";
constexpr const char* color_reset = "\033[0m";

#define PRINT_ERROR(...) spdlog::error("[-] " __VA_ARGS__)
#define PRINT_WARNING(...) spdlog::warn("[!] " __VA_ARGS__)
#define PRINT_INFO(...) spdlog::info("[+] " __VA_ARGS__)
#define PRINT_DEBUG_FUNCTION_NAME(flag) \
  do {                                  \
    if (flag) {                         \
      spdlog::info("[+] {}", __func__); \
    }                                   \
  } while (0)

/// @brief addr_tuple holds both virtual address and its physical address
struct addr_tuple {
  char* vaddr;
  uint64_t paddr;
};

// Bitwise XOR reductions
uint64_t XORReductionWithMask(uint64_t mask, uint64_t addr);
uint64_t XORReductionWithMasks(const std::vector<uint64_t>& masks,
                               uint64_t addr);

// Exhaustively generates all possible bitmasks
std::vector<uint64_t> GenerateAllCombinations(uint64_t function);

// Histogram
uint64_t** AllocateHistogram(size_t num_rows, size_t num_cols);
void FreeHistogram(uint64_t** histogram, size_t num_rows);

// Statistics
uint64_t GetMedian(uint64_t** histogram, uint64_t num_rows, uint64_t cidx);
uint64_t GetMedian(std::vector<uint64_t>& values);
uint64_t GetAverage(uint64_t** histogram, uint64_t num_rows, uint64_t cidx);
uint64_t GetAverage(std::vector<uint64_t>& values);
uint64_t GetMinimum(uint64_t** histogram, uint64_t num_rows, uint64_t cidx);
uint64_t GetMinimum(std::vector<uint64_t>& values);
uint64_t GetMaximum(uint64_t** histogram, uint64_t num_rows, uint64_t cidx);
uint64_t GetMaximum(std::vector<uint64_t>& values);
void GetStatistics(uint64_t** histogram, uint64_t num_rows, uint64_t cidx,
                   uint64_t* results);
void GetStatistics(std::vector<uint64_t>& values, uint64_t* results);

// Gaussian elimination to solve the system of the linear equations over GF(2)
// refer to
// https://graphics.stanford.edu/~seander/bithacks.html#NextBitPermutation
uint64_t NextBitPermutation(uint64_t v);
// refer to https://www.cs.umd.edu/~gasarch/TOPICS/factoring/fastgauss.pdf
std::vector<uint64_t> ReduceFunctions(std::vector<uint64_t> functions);

}  // namespace sudoku

#endif  // SUDOKU_INTERNAL_UTILS_H