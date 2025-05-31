#include "config.h"

#include <cmath>

namespace sudoku {

uint64_t GetNumRanks(MemoryConfig* config) {
  return config->num_mcs * config->num_channels_per_mc *
         config->num_dimms_per_channel *
         (1ULL << config->dram_config->num_subchannel_bits) *
         (1ULL << config->dram_config->num_rank_bits);
}

uint64_t GetNumBanks(MemoryConfig* config) {
  return GetNumRanks(config) *
         (1ULL << config->dram_config->num_bank_group_bits) *
         (1ULL << config->dram_config->num_bank_address_bits);
}

uint64_t GetNumFunctions(MemoryConfig* config) {
  return static_cast<uint64_t>(std::log2(GetNumBanks(config)));
}

uint64_t GetNumRows(MemoryConfig* config) {
  return 1ULL << config->dram_config->num_row_bits;
}

uint64_t GetNumColumns(MemoryConfig* config) {
  return 1ULL << config->dram_config->num_column_bits;
}

uint64_t GetNumRankDimms(MemoryConfig* config) {
  return config->num_dimms_per_channel *
         (1ULL << config->dram_config->num_rank_bits);
}

}  // namespace sudoku