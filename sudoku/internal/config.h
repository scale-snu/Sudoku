#ifndef SUDOKU_INTERNAL_CONFIG_H
#define SUDOKU_INTERNAL_CONFIG_H

#include <cmath>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

#include "utils.h"

namespace sudoku {

// Constants
constexpr uint64_t KB = 1024ULL;
constexpr uint64_t MB = 1024ULL * KB;
constexpr uint64_t GB = 1024ULL * MB;

// DRAM types
enum class DDRType {
  DDR4 = 0,
  DDR5,
  UNKNOWN,
};

static const std::string DDRTypeStr[] = {
    "DDR4",
    "DDR5",
    "UNKNOWN",
};

// DRAM configuration table
struct AddressTableEntry {
  DDRType type;
  uint64_t chip_size;
  uint16_t dq;
  uint16_t num_bank_group_bits;
  uint16_t num_bank_address_bits;
  uint32_t num_row_bits;
  uint32_t num_column_bits;
  uint32_t page_size;
  uint32_t burst_length;
};

static constexpr AddressTableEntry DRAMAddressTable[] = {
    // {type, chip_size, dq, bg, ba, row, column, page_size, burst_length}
    {DDRType::DDR4, 8ULL * GB, 8, 2, 2, 16, 10, 8 * 1024, 8},
    {DDRType::DDR4, 16ULL * GB, 8, 2, 2, 17, 10, 8 * 1024, 8},
    {DDRType::DDR5, 16ULL * GB, 8, 3, 2, 16, 10, 8 * 1024, 16},
};

static constexpr uint32_t kNumChipEntries =
    sizeof(DRAMAddressTable) / sizeof(AddressTableEntry);

// DRAM configuration
struct DRAMConfig {
  DDRType type;
  uint64_t module_size;
  uint16_t num_ranks;
  uint16_t dq;
  uint64_t chip_size;
  uint32_t num_rank_bits;
  uint32_t num_subchannel_bits;
  uint32_t num_bank_group_bits;
  uint32_t num_bank_address_bits;
  uint32_t num_row_bits;
  uint32_t num_column_bits;

  DRAMConfig()
      : type(DDRType::DDR4),
        module_size(32ULL * GB),
        num_ranks(2),
        dq(8),
        chip_size(16ULL * GB),
        num_rank_bits(1),
        num_subchannel_bits(0),
        num_bank_group_bits(2),
        num_bank_address_bits(2),
        num_row_bits(17),
        num_column_bits(10) {}

  DRAMConfig(DDRType t, uint64_t ms, uint64_t rk, uint16_t d)
      : type(t), module_size(ms), num_ranks(rk), dq(d) {
    // for DDR modules
    chip_size = module_size / (num_ranks * 64 / dq) * 8;

    int idx = -1;
    for (uint32_t i = 0; i < kNumChipEntries; ++i) {
      const auto& entry = DRAMAddressTable[i];
      if (entry.type == type && entry.chip_size == chip_size &&
          entry.dq == dq) {
        idx = i;
        break;
      }
    }

    if (idx == -1) {
      PRINT_ERROR("[-] Cannot find DRAM configuration in the table\n");
      exit(EXIT_FAILURE);
    }

    const auto& entry = DRAMAddressTable[idx];
    num_subchannel_bits = (type == DDRType::DDR4) ? 0 : 1;
    num_rank_bits = std::log2(num_ranks);
    num_bank_group_bits = entry.num_bank_group_bits;
    num_bank_address_bits = entry.num_bank_address_bits;
    num_row_bits = entry.num_row_bits;
    num_column_bits = entry.num_column_bits - std::log2(entry.burst_length);
  }

  std::string ToString() const {
    std::ostringstream oss;
    oss << DDRTypeStr[static_cast<int>(type)] << "," << num_ranks << "Rx" << dq
        << "," << (module_size / GB) << "GB";
    return oss.str();
  }
};

// Memory system configuration
struct MemoryConfig {
  uint32_t num_mcs;
  uint32_t num_channels_per_mc;
  uint32_t num_dimms_per_channel;
  DRAMConfig* dram_config;

  MemoryConfig()
      : num_mcs(1),
        num_channels_per_mc(1),
        num_dimms_per_channel(1),
        dram_config(new DRAMConfig()) {}

  MemoryConfig(uint32_t m, uint32_t c, uint32_t d, DRAMConfig* cfg)
      : num_mcs(m),
        num_channels_per_mc(c),
        num_dimms_per_channel(d),
        dram_config(cfg) {}

  std::string ToString() const {
    std::ostringstream oss;
    for (uint32_t i = 0; i < num_mcs; ++i) {
      for (uint32_t j = 0; j < num_channels_per_mc; ++j) {
        for (uint32_t k = 0; k < num_dimms_per_channel; ++k) {
          oss << "MC" << i << ",CH" << j << ",DIMM" << k
              << dram_config->ToString() << "\n";
        }
      }
    }
    return oss.str();
  }
};

uint64_t GetNumRanks(MemoryConfig* config);
uint64_t GetNumBanks(MemoryConfig* config);
uint64_t GetNumFunctions(MemoryConfig* config);
uint64_t GetNumRows(MemoryConfig* config);
uint64_t GetNumColumn(MemoryConfig* config);

uint64_t GetNumRankDimms(MemoryConfig* config);

}  // namespace sudoku

#endif  // SUDOKU_INTERNAL_CONFIG_H
