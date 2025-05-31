#ifndef SUDOKU_SUDOKU_H
#define SUDOKU_SUDOKU_H

#include <spdlog/spdlog.h>

#include <string>
#include <vector>

#include "config.h"
#include "memory.h"
#include "pool.h"
#include "utils.h"

namespace sudoku {

/// @brief Base class for reverse-engineering the commercial processors' memory
/// controller
class Sudoku {
 public:
  Sudoku() = delete;
  Sudoku(const Sudoku&) = delete;
  Sudoku& operator=(const Sudoku&) = delete;

  Sudoku(DRAMConfig* dram_config, MemoryConfig* memory_config,
         MemoryPoolConfig* memory_pool_config, std::string type,
         std::string fname_prefix, bool verbose, bool logging, bool debug);
  virtual ~Sudoku() = default;

  void Initialize();
  void Finalize();

  // Logger
  void SetupLogger(std::string fname, std::string log_name);

  // Setters (configurations)
  void SetDRAMConfig(DRAMConfig* config);
  void SetMemoryConfig(MemoryConfig* config);
  void SetMemoryPoolConfig(MemoryPoolConfig* config);

  // Setters (functions and bits)
  void SetAddressingFunctions(std::vector<uint64_t> functions);
  void SetChannelFunctions(std::vector<uint64_t> functions);
  void SetRankFunctions(std::vector<uint64_t> functions);
  void SetBankFunctions(std::vector<uint64_t> functions);
  void SetBankGroupFunctions(std::vector<uint64_t> functions);
  void SetBankAddressFunctions(std::vector<uint64_t> functions);
  void SetRowBits(uint64_t bits);
  void SetColumnBits(uint64_t bits);

  // Getters (functions and bits)
  std::vector<uint64_t> GetChannelFunctions() const;
  std::vector<uint64_t> GetRankFunctions() const;
  std::vector<uint64_t> GetBankFunctions() const;
  std::vector<uint64_t> GetBankGroupFunctions() const;
  std::vector<uint64_t> GetBankAddressFunctions() const;
  uint64_t GetRowBits() const;
  uint64_t GetColumnBits() const;

  //
  uint64_t GetNumFunctions() const;
  uint64_t GetNumRowBits() const;
  uint64_t GetNumColumnBits() const;
  uint64_t GetNumRankDimmFunctions() const;
  uint64_t GetNumBankAddressFunctions() const;

  // Address generation
  void GenerateRandomAddressTuple(addr_tuple* tuple);
  void GenerateTwoRandomAddressTuples(addr_tuple* first, addr_tuple* second);
  bool GenerateRandomAddressTupleWithConstraints(
      addr_tuple* first, addr_tuple* second,
      const std::vector<uint64_t>& same_functions,
      const std::vector<uint64_t>& diff_functions);
  std::vector<uint64_t> GenerateRowBufferHitSequences(uint64_t length);

 public:
  MemoryPool* pool_;

 protected:
  DRAMConfig* dram_config_;
  MemoryConfig* memory_config_;
  MemoryPoolConfig* memory_pool_config_;

  std::string type_;
  std::string fname_prefix_;
  uint64_t max_bits_;
  bool verbose_;
  bool logging_;
  bool debug_;

  // functions and bits
  std::vector<uint64_t> addressing_functions_;
  std::vector<uint64_t> channel_functions_;
  std::vector<uint64_t> rank_functions_;
  std::vector<uint64_t> bank_functions_;
  std::vector<uint64_t> bank_group_functions_;
  std::vector<uint64_t> bank_address_functions_;
  std::vector<uint64_t> row_functions_;
  std::vector<uint64_t> column_functions_;
  uint64_t row_bits_;
  uint64_t column_bits_;
};

}  // namespace sudoku

#endif  // SUDOKU_SUDOKU_H