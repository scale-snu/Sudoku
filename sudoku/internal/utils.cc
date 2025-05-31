#include "utils.h"

#include <algorithm>
#include <bitset>
#include <cmath>
#include <numeric>
#include <vector>

#include "constants.h"

namespace sudoku {

uint64_t XORReductionWithMask(uint64_t mask, uint64_t addr) {
  return __builtin_parityl(mask & addr);
}

uint64_t XORReductionWithMasks(const std::vector<uint64_t>& masks,
                               uint64_t addr) {
  uint64_t retval = 0, bitpos = 0;
  for (const auto& mask : masks) {
    retval |= (__builtin_parityl(mask & addr) << (bitpos++));
  }
  return retval;
}

std::vector<uint64_t> GenerateAllCombinations(uint64_t function) {
  std::vector<uint64_t> ret = {0};
  uint64_t bit = 1;
  while (bit <= function) {
    if (function & bit) {
      std::vector<uint64_t> temp;
      for (auto val : ret) {
        temp.push_back(val + bit);
      }
      ret.insert(ret.end(), temp.begin(), temp.end());
    }
    bit <<= 1;
  }
  auto it = std::find(ret.begin(), ret.end(), 0);
  if (it != ret.end()) {
    ret.erase(it);
  }
  return ret;
}

uint64_t** AllocateHistogram(size_t num_rows, size_t num_cols) {
  uint64_t** histogram = new uint64_t*[num_rows];
  for (size_t i = 0; i < num_rows; ++i) {
    histogram[i] = new uint64_t[num_cols];
  }
  return histogram;
}

void FreeHistogram(uint64_t** histogram, size_t num_rows) {
  if (!histogram) {
    return;
  }
  for (size_t i = 0; i < num_rows; ++i) {
    delete[] histogram[i];
  }
  delete[] histogram;
}

uint64_t GetMedian(uint64_t** histogram, uint64_t num_rows, uint64_t cidx) {
  std::vector<uint64_t> columns(num_rows);
  for (uint64_t i = 0; i < num_rows; ++i) {
    columns[i] = histogram[i][cidx];
  }
  std::sort(columns.begin(), columns.end());
  return (num_rows % 2 == 0)
             ? (columns[num_rows / 2 - 1] + columns[num_rows / 2]) / 2
             : columns[num_rows / 2];
}

uint64_t GetMedian(std::vector<uint64_t>& values) {
  if (values.empty()) {
    return 0;
  }
  std::sort(values.begin(), values.end());
  size_t n = values.size();
  return (n % 2 == 0) ? (values[n / 2 - 1] + values[n / 2]) / 2 : values[n / 2];
}

uint64_t GetAverage(uint64_t** histogram, uint64_t num_rows, uint64_t cidx) {
  uint64_t sum = 0;
  for (uint64_t i = 0; i < num_rows; ++i) {
    sum += histogram[i][cidx];
  }
  return sum / num_rows;
}

uint64_t GetAverage(std::vector<uint64_t>& values) {
  if (values.empty()) {
    return 0;
  }
  uint64_t sum = std::accumulate(values.begin(), values.end(), uint64_t(0));
  return sum / values.size();
}

uint64_t GetMinimum(uint64_t** histogram, uint64_t num_rows, uint64_t cidx) {
  uint64_t min_val = histogram[0][cidx];
  for (uint64_t i = 1; i < num_rows; ++i) {
    min_val = std::min(min_val, histogram[i][cidx]);
  }
  return min_val;
}

uint64_t GetMinimum(std::vector<uint64_t>& values) {
  if (values.empty()) {
    return 0;
  }
  std::sort(values.begin(), values.end());
  return values[0];
}

uint64_t GetMaximum(uint64_t** histogram, uint64_t num_rows, uint64_t cidx) {
  uint64_t max_val = histogram[0][cidx];
  for (uint64_t i = 1; i < num_rows; ++i) {
    max_val = std::max(max_val, histogram[i][cidx]);
  }
  return max_val;
}

uint64_t GetMaximum(std::vector<uint64_t>& values) {
  if (values.empty()) {
    return 0;
  }
  std::sort(values.begin(), values.end());
  return values[values.size() - 1];
}

void GetStatistics(uint64_t** histogram, uint64_t num_rows, uint64_t cidx,
                   uint64_t* results) {
  std::vector<uint64_t> columns(num_rows);
  for (uint64_t i = 0; i < num_rows; ++i) {
    columns[i] = histogram[i][cidx];
  }
  // sort
  std::sort(columns.begin(), columns.end());
  results[0] =
      std::accumulate(columns.begin(), columns.end(), uint64_t(0)) / num_rows;
  results[1] = (num_rows % 2 == 0)
                   ? (columns[num_rows / 2 - 1] + columns[num_rows / 2]) / 2
                   : columns[num_rows / 2];
  results[2] = columns.front();
  results[3] = columns.back();
}

void GetStatistics(std::vector<uint64_t>& values, uint64_t* results) {
  if (values.empty()) {
    std::fill(results, results + 4, 0);
    return;
  }
  std::sort(values.begin(), values.end());
  results[0] = GetAverage(values);
  results[1] = GetMedian(values);
  results[2] = values.front();
  results[3] = values.back();
}

// Ref https://graphics.stanford.edu/~seander/bithacks.html#NextBitPermutation
// Generate next bit permutation pattern (assuming fixed number of 1s)
uint64_t NextBitPermutation(uint64_t v) {
  uint64_t t = v | (v - 1);
  return (t + 1) | (((~t & -~t) - 1) >> (__builtin_ctzl(v) + 1));
}

// Ref https://www.cs.umd.edu/~gasarch/TOPICS/factoring/fastgauss.pdf
// Gaussian elimination in GF2 from 'TRRespass' GitHub
std::vector<uint64_t> ReduceFunctions(std::vector<uint64_t> functions) {
  uint64_t h, w, h_t, w_t;
  h = functions.size();
  w = 0;
  for (auto f : functions) {
    uint64_t max = 64 - __builtin_clzl(f);
    w = (max > w) ? max : w;
  }
  h_t = w;
  w_t = h;
  std::vector<std::vector<bool>> mtx(h, std::vector<bool>(w));
  std::vector<std::vector<bool>> mtx_t(h_t, std::vector<bool>(w_t));
  std::vector<uint64_t> filtered;
  for (uint64_t i = 0; i < h; i++) {
    for (uint64_t j = 0; j < w; j++) {
      mtx[i][w - j - 1] = (functions[i] & (1ULL << (j)));
    }
  }
  for (uint64_t i = 0; i < h; i++) {
    for (uint64_t j = 0; j < w; j++) {
      mtx_t[j][i] = mtx[i][j];
    }
  }
  uint64_t pvt_col = 0;
  while (pvt_col < w_t) {
    for (uint64_t row = 0; row < h_t; row++) {
      if (mtx_t[row][pvt_col]) {
        filtered.push_back(functions[pvt_col]);
        for (uint64_t c = 0; c < w_t; c++) {
          if (c == pvt_col) continue;
          if (!(mtx_t[row][c])) continue;
          // column sum
          for (uint64_t r = 0; r < h_t; r++) {
            mtx_t[r][c] = (mtx_t[r][c] != mtx_t[r][pvt_col]);
          }
        }
        break;
      }
    }
    pvt_col++;
  }
  return filtered;
}

}  // namespace sudoku