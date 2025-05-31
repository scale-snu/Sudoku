#include "sudoku.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <utility>

#include "internal/assembly.h"
#include "internal/constants.h"

namespace sudoku {

Sudoku::Sudoku(DRAMConfig* dram_config, MemoryConfig* memory_config,
               MemoryPoolConfig* memory_pool_config, std::string type,
               std::string fname_prefix, bool verbose, bool logging, bool debug)
    : dram_config_(dram_config),
      memory_config_(memory_config),
      memory_pool_config_(memory_pool_config),
      type_(type),
      fname_prefix_(fname_prefix),
      verbose_(verbose),
      logging_(logging),
      debug_(debug) {
  if (dram_config_ == nullptr || memory_config_ == nullptr) {
    dram_config_ = new DRAMConfig();
    memory_config_ = new MemoryConfig();
  }
  max_bits_ =
      (uint64_t)std::log2(dram_config_->module_size * memory_config_->num_mcs *
                          memory_config_->num_channels_per_mc *
                          memory_config_->num_dimms_per_channel);
  pool_ = new MemoryPool(memory_pool_config_);
}

void Sudoku::Initialize() {
  if (!InitMemoryPool(pool_)) {
    PRINT_ERROR("Sudoku::Initialize() failed");
    exit(EXIT_FAILURE);
  }
  if (verbose_) {
    PRINT_INFO("Sudoku::Initialize() memory pool at: ");
    for (uint64_t i = 0; i < memory_pool_config_->num_pages; ++i) {
      PRINT_INFO("Pool {}{}{}{}{}", (i + 1), ",",
                 reinterpret_cast<void*>(pool_->pages[i]), ",",
                 reinterpret_cast<void*>(
                     VirtToPhys(reinterpret_cast<uint64_t>(pool_->pages[i]))));
    }
  }
}

void Sudoku::Finalize() {
  if (!FreeMemoryPool(pool_)) {
    PRINT_ERROR("Sudoku::Finalize() failed");
    exit(EXIT_FAILURE);
  }
}

void Sudoku::SetupLogger(std::string fname, std::string log_name) {
  auto print_sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
  auto file_sink =
      std::make_shared<spdlog::sinks::basic_file_sink_st>(fname, true);
  // Check flags
  std::vector<spdlog::sink_ptr> sinks;
  if (verbose_) {
    sinks.push_back(print_sink);
  }
  if (logging_) {
    sinks.push_back(file_sink);
  }
  // Setup logger
  auto logger =
      std::make_shared<spdlog::logger>(log_name, sinks.begin(), sinks.end());
  logger->set_level(spdlog::level::info);
  spdlog::register_logger(logger);
}

void Sudoku::SetDRAMConfig(DRAMConfig* config) { dram_config_ = config; }

void Sudoku::SetMemoryConfig(MemoryConfig* config) { memory_config_ = config; }

void Sudoku::SetMemoryPoolConfig(MemoryPoolConfig* config) {
  memory_pool_config_ = config;
}

void Sudoku::SetAddressingFunctions(std::vector<uint64_t> functions) {
  addressing_functions_ = functions;
}

void Sudoku::SetChannelFunctions(std::vector<uint64_t> functions) {
  channel_functions_ = functions;
}

void Sudoku::SetRankFunctions(std::vector<uint64_t> functions) {
  rank_functions_ = functions;
}

void Sudoku::SetBankFunctions(std::vector<uint64_t> functions) {
  bank_functions_ = functions;
}

void Sudoku::SetBankGroupFunctions(std::vector<uint64_t> functions) {
  bank_group_functions_ = functions;
}

void Sudoku::SetBankAddressFunctions(std::vector<uint64_t> functions) {
  bank_address_functions_ = functions;
}

void Sudoku::SetRowBits(uint64_t bits) { row_bits_ = bits; }

void Sudoku::SetColumnBits(uint64_t bits) { column_bits_ = bits; }

std::vector<uint64_t> Sudoku::GetChannelFunctions() const {
  return channel_functions_;
}

std::vector<uint64_t> Sudoku::GetRankFunctions() const {
  return rank_functions_;
}

std::vector<uint64_t> Sudoku::GetBankFunctions() const {
  return bank_functions_;
}

std::vector<uint64_t> Sudoku::GetBankGroupFunctions() const {
  return bank_group_functions_;
}

std::vector<uint64_t> Sudoku::GetBankAddressFunctions() const {
  return bank_address_functions_;
}

uint64_t Sudoku::GetRowBits() const { return row_bits_; }

uint64_t Sudoku::GetColumnBits() const { return column_bits_; }

uint64_t Sudoku::GetNumFunctions() const {
  return (uint64_t)(std::log2(memory_config_->num_mcs *
                              memory_config_->num_channels_per_mc *
                              memory_config_->num_dimms_per_channel)) +
         dram_config_->num_subchannel_bits + dram_config_->num_rank_bits +
         dram_config_->num_bank_group_bits +
         dram_config_->num_bank_address_bits;
}

uint64_t Sudoku::GetNumRowBits() const { return dram_config_->num_row_bits; }

uint64_t Sudoku::GetNumColumnBits() const {
  return dram_config_->num_column_bits;
}

uint64_t Sudoku::GetNumRankDimmFunctions() const {
  return static_cast<uint64_t>(std::log2(GetNumRankDimms(memory_config_)));
}

uint64_t Sudoku::GetNumBankAddressFunctions() const {
  return dram_config_->num_bank_address_bits;
}

void Sudoku::GenerateRandomAddressTuple(addr_tuple* tuple) {
  uint64_t page_num = pool_->page_dist(pool_->gen);
  uint64_t offset = pool_->page_offset(pool_->gen);
  uint64_t distance =
      (offset / (pool_->config)->granularity) * (pool_->config)->granularity;
  tuple->vaddr = reinterpret_cast<char*>(
      reinterpret_cast<uint64_t>(pool_->pages[page_num]) + distance);
  tuple->paddr = VirtToPhys(reinterpret_cast<uint64_t>(tuple->vaddr));
}

void Sudoku::GenerateTwoRandomAddressTuples(addr_tuple* first,
                                            addr_tuple* second) {
  // Generate first and then second
  GenerateRandomAddressTuple(first);
  do {
    GenerateRandomAddressTuple(second);
  } while (first->vaddr == second->vaddr);
}

bool Sudoku::GenerateRandomAddressTupleWithConstraints(
    addr_tuple* first, addr_tuple* second,
    const std::vector<uint64_t>& same_functions,
    const std::vector<uint64_t>& diff_functions) {
  std::vector<uint64_t> base_functions;
  std::vector<uint8_t> expected;
  // merge functions into matrix and set result vector (expected)
  for (auto function : same_functions) {
    base_functions.push_back(function);
    expected.push_back(
        XORReductionWithMask(function, first->paddr - PCI_OFFSET));
  }
  for (auto function : diff_functions) {
    base_functions.push_back(function);
    expected.push_back(
        !XORReductionWithMask(function, first->paddr - PCI_OFFSET));
  }
  // Solve A * x = b over GF(2)
  std::vector<uint64_t> nullspace;
  std::vector<int> pivot_row(max_bits_, -1);
  size_t row = 0;
  for (int bit = (max_bits_ - 1); bit >= CACHELINE_OFFSET; --bit) {
    int pivot = -1;
    for (size_t i = row; i < base_functions.size(); ++i) {
      if ((base_functions[i] >> bit) & 1) {
        pivot = i;
        break;
      }
    }
    if (pivot == -1) {
      continue;
    }
    std::swap(base_functions[row], base_functions[pivot]);
    std::swap(expected[row], expected[pivot]);
    pivot_row[bit] = row;
    for (size_t i = 0; i < base_functions.size(); ++i) {
      if (i != row && ((base_functions[i] >> bit) & 1)) {
        base_functions[i] ^= base_functions[row];
        expected[i] ^= expected[row];
      }
    }
    ++row;
  }
  for (size_t i = row; i < base_functions.size(); ++i) {
    if (base_functions[i] == 0 && expected[i] != 0) {
      return false;  // No solution
    }
  }
  // Back-substitution for particular solution
  uint64_t solution = 0;
  for (size_t bit = CACHELINE_OFFSET; bit < max_bits_; ++bit) {
    int r = pivot_row[bit];
    if (r == -1) {
      continue;
    }
    uint8_t rhs = expected[r];
    for (size_t j = bit + 1; j < max_bits_; ++j) {
      if ((base_functions[r] >> j) & 1) {
        rhs ^= (solution >> j) & 1;
      }
    }
    if (rhs) {
      solution |= (1ULL << bit);
    }
  }
  // nullspace basis
  for (size_t bit = CACHELINE_OFFSET; bit < max_bits_; ++bit) {
    if (pivot_row[bit] == -1) {
      uint64_t vec = (1ULL << bit);
      for (size_t j = CACHELINE_OFFSET; j < max_bits_; ++j) {
        int r = pivot_row[j];
        if (r != -1 && ((base_functions[r] >> bit) & 1)) {
          vec |= (1ULL << j);
        }
      }
      nullspace.push_back(vec);
    }
  }
  // Add randomness in solution
  for (auto& ns : nullspace) {
    if (pool_->gen() % 2) {
      solution ^= ns;
    }
  }
  second->paddr = solution + PCI_OFFSET;
  second->vaddr = reinterpret_cast<char*>(PhysToVirt(pool_, second->paddr));
  return (second->vaddr != nullptr);
}

std::vector<uint64_t> Sudoku::GenerateRowBufferHitSequences(uint64_t length) {
  std::vector<uint64_t> sequence(length), generated;
  uint64_t used_bits_mask = 0, unused_column_bits_mask = 0;
  for (const auto& function : addressing_functions_) {
    used_bits_mask |= function;
  }
  // only use "unused" column bits for generating sequences
  unused_column_bits_mask = column_bits_ & ~used_bits_mask;
  // Exhaustive testing to verify all possible bitmasks
  generated = GenerateAllCombinations(unused_column_bits_mask);
  std::shuffle(generated.begin(), generated.end(), pool_->gen);
  std::copy(generated.begin(), generated.begin() + length, sequence.begin());
  return sequence;
}

}  // namespace sudoku
